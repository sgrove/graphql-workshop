/* Example query 1: Hitting data in the database via Hasura */
module Q1 = [%graphql
  {|query find {
  question {
    id
    body
  }
  cc_user {
    id
    name
    votes {
      question {
        body
      }
      cc_user {
        name
      }
    }
  }
}
|}
];

/* Example query 2: Hitting data from npm by way of Hasura -> OneGraph -> npm.
    In this case, Hasura is acting as our GraphQL gateway.
   */
module Q2 = [%graphql
  {|
     query npm($package: String!) {
       npm {
         package(name: $package) {
           homepage
           description
           keywords
           name
           readme
           readmeFilename
           license {
             type
           }
           maintainers {
             name
           }
           versions {
             version
             dependencies {
               name
               version
             }
           }
           downloads {
             lastMonth {
               count
               perDay {
                 day
                 count
               }
             }
           }
           time {
             versions {
               date
               version
             }
           }
           bundlephobia {
             gzip
             size
             history {
               gzip
               size
               version
             }
             dependencySizes {
               approximateSize
               name
             }
           }
           repository {
             url
             sourceRepository {
               ... on GitHubRepository {
                 name
                 url
                 homepageUrl
                 defaultBranchRef {
                   name
                 }
                 stargazers {
                   totalCount
                 }
                 watchers {
                   totalCount
                 }
                 issues(states: OPEN) {
                   totalCount
                 }
                 forks {
                   totalCount
                 }
                 pullRequests(states: OPEN) {
                   totalCount
                 }
                 primaryLanguage {
                   name
                   color
                 }
               }
             }
           }
         }
       }
     }|}
];

open Cohttp_async;

let serverUri = Uri.of_string("http://localhost:8080/v1alpha1/graphql");

let uuidOf = (json: Yojson.Basic.json) => {
  switch (json) {
  | `String(uuid) => Uuidm.of_string(uuid)
  | _ => None
  };
};

let makeAuthHeader = (~secret) =>
  Cohttp.Header.init_with("x-hasura-admin-secret", secret);

let call = (~headers=?, uri, p) => {
  open Async;

  let queryBody =
    `Assoc([("query", `String(p#query)), ("variables", p#variables)]);

  Workshop.OneLog.infof(
    "Raw pretty-printend GraphQL request:\n%s\n",
    Yojson.Basic.pretty_to_string(queryBody),
  );

  Client.post(
    ~headers?,
    ~body=queryBody |> Yojson.Basic.to_string |> Cohttp_async.Body.of_string,
    uri,
  )
  >>= (
    ((_resp, bodyBytes)) => {
      Cohttp_async.Body.to_string(bodyBytes)
      >>| (
        body => {
          let json = Yojson.Basic.from_string(body);

          Workshop.OneLog.infof(
            "Raw pretty-printend GraphQL json:\n%s\n",
            Yojson.Basic.pretty_to_string(json),
          );

          switch (
            Yojson.Basic.Util.(member("data", json), member("errors", json))
          ) {
          | (`Assoc(data), _) => Ok(p#parse(`Assoc(data)))
          | (`Null, `List(data)) => Error(`List(data))
          | _ => Error(`List([]))
          };
        }
      );
    }
  );
};

let testHasura = (~secret) => {
  open Async;
  let p = Q1.make();

  call(~headers=makeAuthHeader(~secret), serverUri, p)
  >>| (
    fun
    | Error(err) =>
      Workshop.OneLog.infof(
        "Error in GraphQL call:\n%s\n",
        Yojson.Basic.pretty_to_string(err),
      )
    | Ok(json) => {
        Yojson.Basic.Util.(
          json#cc_user
          |> Array.iter(user =>
               Workshop.OneLog.infof(
                 "CC user name: %s (%s)\n",
                 user#name,
                 Workshop.Ooption.fmapDefault(
                   "No id",
                   uuidOf(user#id),
                   Uuidm.to_string,
                 ),
               )
             )
        );
      }
  );
};

/* A convenience method that wraps up all the Hasura-specific requirements. */
let hasura = (~authSecret, p) =>
  call(~headers=makeAuthHeader(~secret=authSecret), serverUri, p);
