open Common;

/* A simple heartbeat endpoint to be used with e.g. load balance health check */
let healthCheckHandler = (_keys, _rest, _request) =>
  defer(HttpServer.Util.respondOk("healthy"));

/* Just a test method to see what raising uncaught exceptions results in for the
   server and client */
let throwErrorHandler = (_keys, _rest, _request) =>
  raise(Failure("test-failure"));

/* A handler to execute GraphQL queries */
let dynamicQueryHandler = (~readOnly, _keys, _rest, request) =>
  GraphQLServer.dynamicQuery(~readOnly, request);

/* Actual routes and catch-all handlers */
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

/* Actual routes and catch-all handlers */
let testHandler = (_, _, _) =>
  defer(
    HttpServer.Util.respondOk(
      "Hi there, I'm a test route to prove that the server is working. Feel free to remove me!",
    ),
  );

/* When and exception is raised, we'll provide our own handler for it */
let errorHandler = (_reqd, _request, exn) => {
  Workshop.OneLog.errorf(
    "Handler exception in request: %s\n",
    Core.Exn.to_string(exn),
  )
  |> defer;
};

/* Actual routes and catch-all handlers */
let graphqlTestHandler = (_, _, _) =>
  HttpServer.Util.respondOk(
    "Hi there, I'm a test route to prove that the server is working. Feel free to remove me!",
  )
  |> defer;

let routes = [
  (`GET, "/test", testHandler),
  (`GET, "/graphql_client_test", graphqlTestHandler),
  (`OPTIONS, "/dynamic", HttpServer.Util.allowCORSRequest),
  (`POST, "/dynamic", dynamicQueryHandler(~readOnly=false)),
  (`GET, "/health-check/:check-type", healthCheckHandler),
  (`POST, "/test-error", throwErrorHandler),
  (`POST, "/test-graphql-error", GraphQLServer.throwGraphQLErrorHandler),
];

let dispatch =
  Workshop.OneDispatch.create(routes, notFoundHandler, notImplementedHandler);

let routeHandlers = (request: HttpServer.request) =>
  dispatch(request, request.req.meth, Uri.path(request.uri));
