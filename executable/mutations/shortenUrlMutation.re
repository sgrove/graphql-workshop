open Graphql_async;
open GraphQLUtil;

open Async;

type response = unit;

module US = ShortenedUrlModel;

let responsePayload = () =>
  Schema.(
    obj("ShortenUrlResponsePayload", ~fields=_mutationResponseFields =>
      [
        io_field(
          "shortenedUrl",
          ~args=Arg.[],
          ~typ=string,
          ~resolve=({ctx, _}: GraphQLUtil.rpContext, shortenedResult) =>
          US.byId(~authSecret=ctx.authSecret, ~id=shortenedResult#id)
          >>| (
            fun
            | Some(result) => Ok(Some(result#id))
            | None => Ok(None)
          )
        ),
        io_field(
          "fullUrl",
          ~args=Arg.[],
          ~typ=string,
          ~resolve=({ctx, _}: GraphQLUtil.rpContext, shortenedResult) =>
          US.byId(~authSecret=ctx.authSecret, ~id=shortenedResult#id)
          >>| (
            fun
            | Some(result) =>
              Ok(
                Some(
                  Format.sprintf("%s/short/%s", ctx.serverUrl, result#id),
                ),
              )
            | None => Ok(None)
          )
        ),
      ]
    )
  );

type input = {url: string};

let input =
  Schema.Arg.(
    obj(
      "CreateShortenedUrlInput",
      ~fields=[arg("url", ~typ=non_null(string))],
      ~coerce=url =>
      {url: url}
    )
  );

let mutation = () =>
  Schema.(
    io_field(
      "createShortenedUrl",
      ~typ=non_null(responsePayload()),
      ~args=Arg.[arg("input", ~typ=non_null(input))],
      ~resolve=({ctx, _}: GraphQLUtil.rpContext, (), input) => {
        if (ctx.readOnly) {
          raise(Failure("Refusing to run mutations in read-only mode"));
        };

        let id = Workshop.Util.randomString(10);

        switch (input.url) {
        | "https://news.ycombinator.com" =>
          Deferred.return(
            Error("We can't shorten links to the orange website, sorry"),
          )
        | notHackerNewsUrl =>
          US.insert(
            ~authSecret=ctx.authSecret,
            ~url=Uri.of_string(notHackerNewsUrl),
            ~id,
          )
          >>| (
            fun
            | None => Error("Error inserting shortened url")
            | Some(result) => Ok(result)
          )
        };
      },
    )
  );
