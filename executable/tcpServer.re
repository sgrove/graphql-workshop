open Async;

let error_handler = (_, ~request=?, error, startResponse) =>
  Httpaf.(
    {
      let responseBody =
        startResponse(
          Headers.of_list([("status", "500"), ("connection", "close")]),
        );
      switch (error) {
      | `Exn(exn) =>
        switch (request) {
        | None => ()
        | Some(request) => ignore()
        };
        Workshop.OneLog.errorf(
          "In ErrorHandler: %s",
          Core.Exn.to_string(exn),
        );
      | #Status.standard as error =>
        Workshop.OneLog.error("ServerHttpaf error_handler:");
        Workshop.OneLog.error(Status.default_reason_phrase(error));
      };
      Body.close_writer(responseBody);
    }
  );

let callback = (_, reqd) => {
  let reqId = Workshop.Util.genUuid();
  Deferred.don't_wait_for(WebRoutes.routes(reqId, reqd));
};

let main = (port, max_accepts_per_batch, ()) => {
  let whereToListen = Tcp.Where_to_listen.of_port(port);
  let makeSocket =
    Tcp.(
      Server.create_sock(
        ~on_handler_error=`Raise,
        ~backlog=10000,
        ~max_connections=10000,
        ~max_accepts_per_batch,
        whereToListen,
      )
    );
  makeSocket(
    Httpaf_async.Server.create_connection_handler(
      ~request_handler=callback,
      ~error_handler,
    ),
  )
  >>= (
    server => {
      Deferred.forever((), () =>
        after(Core.Time.Span.(of_sec(60.0)))
        >>| (
          () => {
            let connectionCount = Tcp.Server.num_connections(server);
            Workshop.OneLog.debugf("conns: %d", connectionCount);
          }
        )
      );
      Workshop.OneLog.infof(
        "Server bound to port, app started, accepting connections now on port %d",
        port,
      );
      Deferred.never();
    }
  );
};
