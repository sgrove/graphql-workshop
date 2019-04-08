open Common;
open Async;

type queryVariables = list((string, Yojson.Basic.json));

type queryBody = {
  query: string,
  variables: queryVariables,
  operationName: option(string),
};

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
            Httpaf.Headers.add_list(
              Httpaf.Headers.empty,
              List.concat([[("Content-Type", "application/json")]]),
            );
          {headers, status: `OK, body: bofs(Yojson.Basic.to_string(json))};
        }
      )

    | Error(err) =>
      let body = Yojson.Basic.to_string(err);
      let headers =
        Httpaf.Headers.of_list([
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
