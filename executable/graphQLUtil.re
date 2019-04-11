open Graphql_async;

type schemaContext = {
  readOnly: bool,
  isAdmin: bool,
  authSecret: string,
  serverUrl: string,
};

let serverTrustedHasuraKey = "myadminsecretkey";

/* In a real app, you would want to retrieve the Hasura key for a given user
   from session storage of some kind */
let makeContext = () => {
  readOnly: false,
  isAdmin: false,
  authSecret: serverTrustedHasuraKey,
  serverUrl: "http://localhost:9255",
};

type rpContext = Schema.resolve_info(schemaContext);

let failPublic = (~message) => raise(Failure(message));

let handleErr = (~message="GraphQL query returned error", result) =>
  switch (result) {
  | Error(_err) => failPublic(~message)
  | Ok(data) => data
  };
