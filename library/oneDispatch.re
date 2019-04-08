type supportedMethod = [ | `GET | `POST | `OPTIONS];

let tableFor = (targetMeth: supportedMethod, routes) =>
  Core.List.filter_map(routes, ~f=((meth, path, handler)) =>
    meth == targetMeth ? Some((path, handler)) : None
  );

let dispatch = (table, notFoundHandler, request, path) =>
  switch (Dispatch.DSL.dispatch(table, path)) {
  | Ok(handler) => handler(request)
  | Error(_) => notFoundHandler()
  };

let create = (routes, notFoundHandler, notImplementedHandler) => {
  let tableForGet = tableFor(`GET, routes);

  let tableForPost = tableFor(`POST, routes);

  let tableForOptions = tableFor(`OPTIONS, routes);

  (request, method, path) =>
    switch (method) {
    | `OPTIONS => dispatch(tableForOptions, notFoundHandler, request, path)
    | `GET => dispatch(tableForGet, notFoundHandler, request, path)
    | `POST => dispatch(tableForPost, notFoundHandler, request, path)
    | _ => notImplementedHandler()
    };
};
