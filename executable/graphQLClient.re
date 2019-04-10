module Q2 = [%graphql {|{
  tester {
    id
  }
}|}];

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

open Cohttp_async;
let call = (uri, p) => {
  open Async;

  let queryBody =
    `Assoc([("query", `String(p#query)), ("variables", p#variables)])
    |> Yojson.Basic.to_string
    |> Cohttp_async.Body.of_string;

  Client.post(~body=queryBody, uri)
  >>= (
    ((_resp, bodyBytes)) => {
      Cohttp_async.Body.to_string(bodyBytes)
      >>| (body => Yojson.Basic.from_string(body));
    }
  );
};

let testOneGraph = (packageName, showMetrics) => {
  open Async;
  let p = Query.make(~package=packageName, ());

  let uri =
    Uri.of_string(
      "https://serve.onegraph.com/dynamic?app_id=0b33e830-7cde-4b90-ad7e-2a39c57c0e11&show_metrics=true",
    );

  call(uri, p)
  >>| (
    json => {
      open Yojson.Basic;
      open Yojson.Basic.Util;

      let p = p#parse(json |> member("data"));

      switch (p#npm#package) {
      | None => Workshop.OneLog.infof("No package \"%s\" found", packageName)
      | Some(package) =>
        let downloads =
          switch (package#downloads#lastMonth) {
          | None => None
          | Some(i) => Some(i#count)
          };

        switch (showMetrics) {
        | false => ()
        | true =>
          Workshop.OneLog.infof(
            "Metrics by api:\n%s\n",
            json
            |> member("extensions")
            |> member("metrics")
            |> pretty_to_string,
          )
        };

        Workshop.OneLog.infof(
          "%s: %s downloads last month\n",
          packageName,
          switch (downloads) {
          | None => "missing"
          | Some(count) => string_of_int(count)
          },
        );
      /* Format.printf("@[<1>%s@ =@ %d@ %s@]@.", "Prix TTC", 100, "Euro"); */
      };
    }
  );
};

let testHasura = (packageName, showMetrics) => {
  open Async;
  let p = Q2.make();

  let uri =
    Uri.of_string(
      "https://serve.onegraph.com/dynamic?app_id=0b33e830-7cde-4b90-ad7e-2a39c57c0e11&show_metrics=true",
    );

  call(uri, p)
  >>| (
    json => {
      open Yojson.Basic;
      open Yojson.Basic.Util;

      let p = p#parse(json |> member("data"));

      p#tester
      |> Array.iter(test => Workshop.OneLog.infof("Tester id: %s\n", test#id));
    }
  );
};
