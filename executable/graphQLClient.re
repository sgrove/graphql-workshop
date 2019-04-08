module Query = [%graphql
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

open Cohttp;
open Cohttp_async;

let p = Query.make(~package=packageName, ());

let queryBody =
  `Assoc([("query", `String(p#query)), ("variables", p#variables)])
  |> Yojson.Basic.to_string
  |> Cohttp_async.Body.of_string;

let body =
  Client.post(
    ~body=queryBody,
    Uri.of_string(
      "https://serve.onegraph.com/dynamic?app_id=0b33e830-7cde-4b90-ad7e-2a39c57c0e11&show_metrics=true",
    ),
  )
  >>= (
    ((_resp, bodyBytes)) => {
      Cohttp_async.Body.to_string(bodyBytes) >>| (body => body);
    }
  );

body
>>| (
  body => {
    open Yojson.Basic;
    open Yojson.Basic.Util;

    let json = from_string(body);

    let p = p#parse(json |> member("data"));

    switch (p#npm#package) {
    | None =>
      print_endline(Format.sprintf("No package \"%s\" found", packageName))
    | Some(package) =>
      let downloads =
        switch (package#downloads#lastMonth) {
        | None => None
        | Some(i) => Some(i#count)
        };

      switch (showMetrics) {
      | false => ()
      | true =>
        Format.printf(
          "Metrics by api:\n%s\n",
          json
          |> member("extensions")
          |> member("metrics")
          |> pretty_to_string,
        )
      };

      Format.printf(
        "%s: %s downloads last month\n",
        packageName,
        switch (downloads) {
        | None => "missing"
        | Some(count) => string_of_int(count)
        },
      );
      Format.printf("\n");
    /* Format.printf("@[<1>%s@ =@ %d@ %s@]@.", "Prix TTC", 100, "Euro"); */
    };
  }
);
