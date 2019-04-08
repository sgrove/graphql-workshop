open Async;

module Startup = {
  let start = (port, ()) => {
    TcpServer.main(port, 1, ());
  };
};

let () =
  Command.async(
    ~summary="Echo POST requests",
    Command.Param.(
      map(
        flag(
          "-p",
          optional_with_default(8080, int),
          ~doc="int Source port to listen on",
        ),
        ~f=(port, ()) =>
        Startup.start(port, ())
      )
    ),
  )
  |> Command.run;
