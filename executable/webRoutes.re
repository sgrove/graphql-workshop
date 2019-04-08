open Common;
open Async;

type queryVariables = list((string, Yojson.Basic.json));

type queryBody = {
  query: string,
  variables: queryVariables,
  operationName: option(string),
};
open Httpaf;
open Httpaf_async;

open Graphql_async;

let createGqlCtx = headers => Deferred.return(Ok());

let jsonErr = (value): result('a, Yojson.Basic.json) =>
  switch (value) {
  | Ok(_) as ok => ok
  | Error(err) => Error(`Assoc([("errors", `List([`String(err)]))]))
  };

let execute' = (resolverContext, _flags, variables, operationName, doc) =>
  Graphql_async.Schema.execute(
    TopResolvers.schema,
    resolverContext,
    ~variables,
    ~operation_name=?operationName,
    doc,
  );

let execute =
    (
      req: Httpaf.Request.t,
      incomingReqId,
      resolverContext: TopResolvers.schemaContext,
      variables,
      operationName,
      query,
    ) => {
  let parsedQuery = Graphql_parser.parse(query);

  /* TODO: Add better error message here */
  let queryDocOrError = jsonErr(parsedQuery);
  switch (parsedQuery) {
  | Ok(_) => Workshop.OneLog.debug("parsedQuery successfully")
  | Error(err) =>
    Workshop.OneLog.debugf("parsedQuery failed to parse: %s", err)
  };

  defer(queryDocOrError)
  >>= (
    docOrError =>
      switch (docOrError) {
      | Ok(doc) =>
        execute'(resolverContext, (), variables, operationName, doc)

      | Error(failureJson) => defer(Error(failureJson))
      }
  );
};

let healthCheckHandler = (_keys, _rest, _request) =>
  defer(HttpServer.Util.respondOk("healthy"));

let throwErrorHandler = (_keys, _rest, _request) =>
  raise(Failure("Test-failure"));

let throwGraphQLErrorHandler = (_keys, _rest, _request) =>
  raise(Failure("Test-graphql-failure"));

let decodeQueryBody = bodyString =>
  try (
    {
      let json = Yojson.Basic.from_string(bodyString);
      switch (json) {
      | `Assoc(a) =>
        switch (
          List.assoc_opt("query", a),
          List.assoc_opt("variables", a),
          List.assoc_opt("operationName", a),
        ) {
        | (None, _, _) =>
          Error({|body must be a JSON object with key "query"|})
        | (Some(`String(query)), variables, operationNameJson) =>
          let operationName =
            Workshop.Ooption.fmap(
              operationNameJson,
              Yojson.Basic.Util.to_string_option,
            );
          switch (variables) {
          | None
          | Some(`Null) => Ok({query, variables: [], operationName})
          | Some(`Assoc(v)) => Ok({query, variables: v, operationName})
          | Some(_) => Error({|"variables" must be a valid JSON object.|})
          };
        | (Some(_), _, _) => Error({|"query" value must be a string|})
        }
      | _ => Error({|body must be a JSON object with key "query"|})
      };
    }
  ) {
  | Yojson.Json_error(_) => Error("body must be valid JSON")
  };

[@deriving yojson]
type basicGqlError = {
  code: int,
  message: string,
};

let graphqlErrorBody = (~code=500, message) => {
  {code, message} |> basicGqlError_to_yojson |> Yojson.Safe.to_string;
};

let render400Gql = (~message, ~code=400, request) => {
  let corsHeaders = HttpServer.Util.getCORSHeaders(request);

  HttpServer.{
    headers: HttpServer.Util.withContentType("application/json", corsHeaders),
    status: `OK,
    body: bofs(graphqlErrorBody(~code, message)),
  };
};

type requestDataForGraphQLQuery = {query: string};

let addExtensionsData =
    (
      ~resultJson: Yojson.Basic.json,
      ~extensionFields: [< | `Assoc(list((string, Yojson.Basic.json)))],
    ) =>
  switch (resultJson) {
  | `Assoc(resultFields) =>
    `Assoc(
      switch (List.assoc_opt("extensions", resultFields)) {
      | None =>
        List.concat([
          resultFields,
          [("extensions", (extensionFields :> Yojson.Basic.json))],
        ])
      | Some(`Assoc(existingExtensions)) =>
        let `Assoc(extensionFields) = extensionFields;
        List.concat([
          List.remove_assoc("extensions", resultFields),
          [
            (
              "extensions",
              `Assoc(List.concat([existingExtensions, extensionFields])),
            ),
          ],
        ]);
      | Some(unexpectedExtensions) =>
        Workshop.OneLog.errorf(
          "Expected extensions to be an assoc, got %s",
          Yojson.Basic.to_string(unexpectedExtensions),
        );
        resultFields; /* Bail on adding extensions */
      },
    )
  | _executionResult =>
    Workshop.OneLog.errorf(
      "Expected assoc, got resultJson=%s",
      Yojson.Basic.to_string(resultJson),
    );
    resultJson; /* Bail on adding extensions */
  };

let handleExecutionResult =
    (~session as _, ~resolverContext, {origin}: HttpServer.request, result)
    : Deferred.t(HttpServer.response) =>
  HttpServer.(
    switch (result) {
    | Ok(data) =>
      (
        switch (data) {
        | `Response(json) => Deferred.return(json)
        | `Stream(reader) => raise(Failure("Subscriptions not implemented"))
        }
      )
      >>| (
        json => {
          let headers =
            Headers.add_list(
              Httpaf.Headers.empty,
              List.concat([[("Content-Type", "application/json")]]),
            );
          {headers, status: `OK, body: bofs(Yojson.Basic.to_string(json))};
        }
      )

    | Error(err) =>
      let body = Yojson.Basic.to_string(err);
      let headers =
        Headers.of_list([
          ("Access-Control-Allow-Origin", origin),
          ("content-type", "application/json"),
          ("Access-Control-Allow-Credentials", "true"),
        ]);
      Workshop.OneLog.errorf("dynamic query error body: %s", body);
      Deferred.return({headers, status: `OK, body: bofs(body)});
    }
  );

let dynamicQuery =
    (~readOnly, {req, reqId, bodyString} as request: HttpServer.request)
    : Deferred.t(HttpServer.response) => {
  let resolverContext = ();

  switch (decodeQueryBody(bodyString)) {
  | Error(e) =>
    defer(render400Gql(~message="Error decoding request body: " ++ e, req))
  | Ok({query, variables, operationName}) =>
    execute(
      req,
      reqId,
      resolverContext,
      (variables :> list((string, Graphql_parser.const_value))),
      operationName,
      query,
    )
    >>= (
      result =>
        handleExecutionResult(~session=(), ~resolverContext, request, result)
    )
  };
};

let dynamicQueryHandler = (~readOnly, _keys, _rest, request) =>
  dynamicQuery(~readOnly, request);

let routes = [
  (`OPTIONS, "/dynamic", HttpServer.Util.allowCORSRequest),
  (`POST, "/dynamic", dynamicQueryHandler(~readOnly=false)),
  (
    `GET,
    "/test",
    (_, _, _) => defer(HttpServer.Util.respondOk("Hi there")),
  ),
  (`GET, "/health-check/:check-type", healthCheckHandler),
  (`POST, "/test-error", throwErrorHandler),
  (`POST, "/test-graphql-error", throwGraphQLErrorHandler),
];

let notFoundHandler = () =>
  defer(
    HttpServer.Util.respondOk(
      ~headers=HttpServer.Util.contentTypeHeader("text/html"),
      "We can't find that route, sorry",
    ),
  );

let notImplementedHandler = () =>
  HttpServer.(
    defer({
      headers: Httpaf.Headers.empty,
      status: `Not_implemented,
      body: bofs(""),
    })
  );

let dispatch =
  Workshop.OneDispatch.create(routes, notFoundHandler, notImplementedHandler);

let handlers = (request: HttpServer.request) =>
  dispatch(request, request.req.meth, Uri.path(request.uri));

let shouldUseGzip = reqd => {
  let encoding =
    Httpaf.Headers.get(Reqd.request(reqd).headers, "accept-encoding");
  switch (encoding) {
  | None => false
  | Some(encoding') =>
    Str.string_match(Str.regexp(".*gzip.*"), encoding', 0)
  };
};

let respond = (reqd, {headers, status, body}: HttpServer.response) =>
  switch (body) {
  | `BigString(content) =>
    let useGzip = shouldUseGzip(reqd);
    let bodyString =
      useGzip
        ? Ezgzip.compress(HttpServer.stringOfBigstring(content))
        : HttpServer.stringOfBigstring(content);
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

let handleHandlerError = (reqd, request, exn) => {
  Workshop.OneLog.errorf(
    "Handler exception in request: %s\n",
    Core.Exn.to_string(exn),
  )
  |> defer;
};

let routes = (reqId, reqd) => {
  let req = Reqd.request(reqd);
  let origin = HttpServer.Util.requestOrigin(req);
  let uri = Uri.of_string(req.target);
  let start = Unix.gettimeofday();

  HttpServer.(
    readBody(reqd)
    >>= (
      bodyString => {
        let request = {reqId, req, origin, bodyString, uri};
        try_with(
          ~extract_exn=false,
          ~name="Server",
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
            response =>
              respond(reqd, response) >>| (() => (request, response))
          )
        )
        >>= (
          result =>
            (
              switch (result) {
              | Ok((request, response)) =>
                Deferred.return(Ok((request, response)))
              | Error(exn) =>
                handleHandlerError(reqd, request, exn)
                >>| (
                  () =>
                    Error((
                      request,
                      (
                        Httpaf.Headers.empty,
                        `Internal_server_error,
                        bofs(""),
                      ),
                    ))
                )
              }
            )
            >>| (
              fun
              | Ok((request, {status}))
              | Error((request, (_, status, _))) => {
                  Workshop.OneLog.infof(
                    "Request end method=%s path=%s req_id=%s req_ms=%0.2f",
                    Method.to_string(req.meth),
                    Uri.path(uri),
                    Uuidm.to_string(reqId),
                    (Unix.gettimeofday() -. start) *. 1000.0,
                  );
                }
            )
        );
      }
    )
  );
};
