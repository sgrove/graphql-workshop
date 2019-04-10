open Common;
open Async;

type request = {
  reqId: Uuidm.t,
  req: Httpaf.Request.t,
  origin: string,
  bodyString: string,
  uri: Uri.t,
};

type responseBody = [
  | `BigString(Bigstringaf.t)
  | `Pipe(Pipe.Reader.t(string))
];

type response = {
  headers: Httpaf.Headers.t,
  status: Httpaf.Status.t,
  body: responseBody,
};

open Httpaf;

let bofs = x =>
  `BigString(Bigstringaf.of_string(~off=0, ~len=String.length(x), x));

let stringOfBigstring = (~offset as src_off=0, ~length=?, x) => {
  let length =
    switch (length) {
    | None => Bigstringaf.length(x)
    | Some(len) => len
    };

  let bytes = Bytes.create(length);
  Bigstringaf.blit_to_bytes(x, ~src_off, ~dst_off=0, ~len=length, bytes);

  Bytes.to_string(bytes);
};

module Util = {
  let contentLengthHeader = content =>
    Headers.of_list([
      ("content-length", string_of_int(Bigstring.length(content))),
    ]);

  let contentTypeHeader = kind => Headers.of_list([("content-type", kind)]);

  let _withContentType = (kind, headers) =>
    Headers.add(headers, "content-type", kind);

  let withContentType = (kind, headers) =>
    Headers.add(headers, "content-type", kind);

  let sessionlessHeaders = Httpaf.Headers.empty;

  let respondOk = (~headers=Httpaf.Headers.empty, body) => {
    headers,
    status: `OK,
    body: bofs(body),
  };

  let requestOrigin = (req: Httpaf.Request.t) =>
    switch (Httpaf.Headers.get(req.headers, "origin")) {
    | None => "unknown"
    | Some(origin) => origin
    };

  let getCORSHeaders = req => {
    let origin = requestOrigin(req);
    let headers =
      Headers.of_list([
        (
          "Access-Control-Allow-Headers",
          "content-type,app-id,auth-token,show_beta_schema,authentication,cookie",
        ),
        ("Access-Control-Allow-Methods", "POST"),
        ("Access-Control-Allow-Credentials", "true"),
        ("Access-Control-Allow-Origin", origin),
      ]);
    headers;
  };

  let allowCORSRequest = (_keys, _rest, request) => {
    let {req, _} = request;
    let headers = getCORSHeaders(req);
    defer(respondOk(~headers, ""));
  };

  let shouldUseGzip = reqd => {
    let encoding =
      Httpaf.Headers.get(Reqd.request(reqd).headers, "accept-encoding");
    switch (encoding) {
    | None => false
    | Some(encoding') =>
      Str.string_match(Str.regexp(".*gzip.*"), encoding', 0)
    };
  };
};

let readBody = reqd => {
  let requestBody = Reqd.request_body(reqd);

  let buffer = Buffer.create(16);
  let result = Ivar.create();

  let rec scheduleRead = () =>
    Body.schedule_read(
      requestBody,
      ~on_read=
        (content: Bigstringaf.t, ~off as offset, ~len as length) => {
          Buffer.add_string(
            buffer,
            stringOfBigstring(~offset, ~length, content),
          );
          scheduleRead();
        },
      ~on_eof=
        () => {
          Body.close_reader(requestBody);
          Ivar.fill(result, Buffer.contents(buffer));
        },
    );

  scheduleRead();

  Ivar.read(result);
};

let respond = (reqd, {headers, status, body}: response) =>
  switch (body) {
  | `BigString(content) =>
    let useGzip = Util.shouldUseGzip(reqd);
    let bodyString =
      useGzip
        ? Ezgzip.compress(stringOfBigstring(content))
        : stringOfBigstring(content);
    let headers =
      Httpaf.Headers.add_list(
        headers,
        List.concat([
          [
            ("connection", "close"),
            ("Content-Length", string_of_int(String.length(bodyString))),
          ],
          useGzip ? [("Content-Encoding", "gzip")] : [],
        ]),
      );
    let response =
      Reqd.respond_with_streaming(reqd, Response.create(~headers, status));
    Body.write_string(response, bodyString);
    Body.close_writer(response);
    Deferred.return();
  | `Pipe(p) =>
    let headers =
      Httpaf.Headers.add_list(headers, [("connection", "close")]);
    let response =
      Reqd.respond_with_streaming(reqd, Response.create(~headers, status));

    let rec readPipe = () =>
      Pipe.read(p)
      >>= (
        fun
        | `Eof => Deferred.return()
        | `Ok(content) => {
            Body.write_string(response, content);
            let wait = Ivar.create();
            Body.flush(response, () => Ivar.fill(wait, ()));
            Ivar.read(wait) >>= readPipe;
          }
      );

    readPipe() >>| (() => Body.close_writer(response));
  };

let routes = (handlers, ~errorHandler, reqId: Uuidm.t, reqd) => {
  let req = Httpaf.Reqd.request(reqd);
  let origin = Util.requestOrigin(req);
  let uri = Uri.of_string(req.target);
  let start = Unix.gettimeofday();

  /* Our main http request life-cycle:
     1. Read the body fully from the request into memory - note that this is bad for cases like streaming
     2. Create a more ergonomic representation of all of the request data
     3. Pass the data from #2 to our route handler for a full response
     4. Examine the response to see if it's an error, and if so, handle it appropriately. This is important for GraphQL and CORS requests, as the errors in that situation can be extremely opaque for frontend devs
     5. Pass the response to the client, optionally logging some metrics
     */
  readBody(reqd)
  >>= (
    bodyString => {
      let request = {reqId, req, origin, bodyString, uri};
      try_with(
        ~extract_exn=false,
        ~name="WebRoutes",
        ~rest=
          `Call(
            exn_ =>
              Workshop.OneLog.infof(
                "Unexpected and non-understood exception raised after route result was returned: %s",
                Core.Exn.to_string(exn_),
              ),
          ),
        ~run=`Now,
        () =>
        handlers(request)
        >>= (
          response => respond(reqd, response) >>| (() => (request, response))
        )
      )
      >>= (
        result =>
          (
            switch (result) {
            | Ok((request, response)) =>
              Deferred.return(Ok((request, response)))
            | Error(exn) =>
              errorHandler(reqd, request, exn)
              >>| (
                () =>
                  Error((
                    request,
                    (Httpaf.Headers.empty, `Internal_server_error, bofs("")),
                  ))
              )
            }
          )
          >>| (
            fun
            | Ok((_request, _))
            | Error((_request, (_, _, _))) => {
                Workshop.OneLog.infof(
                  "Request end method=%s path=%s req_id=%s req_ms=%0.2f",
                  Httpaf.Method.to_string(req.meth),
                  Uri.path(uri),
                  Uuidm.to_string(reqId),
                  (Unix.gettimeofday() -. start) *. 1000.0,
                );
              }
          )
      );
    }
  );
};
