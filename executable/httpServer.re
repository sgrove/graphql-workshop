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
open Httpaf_async;

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

module Util = {
  let contentLengthHeader = content =>
    Headers.of_list([
      ("content-length", string_of_int(Bigstring.length(content))),
    ]);

  let contentTypeHeader = kind => Headers.of_list([("content-type", kind)]);

  let withContentType = (kind, headers) =>
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
    let {req} = request;
    let headers = getCORSHeaders(req);
    defer(respondOk(~headers, ""));
  };
};
