open Core;

open Async;

let debugf = Log.Global.debug;

let infof = Log.Global.info;

let warnf = (~tags=[], fmt) =>
  ksprintf(msg => Log.Global.string(~tags, "Warn " ++ msg), fmt);

let errorf = Log.Global.error;

let fatalf = (~tags=[], fmt) =>
  ksprintf(msg => Log.Global.string(~tags, "MUSTFIX " ++ msg), fmt);

let debug = debugf("%s");

let info = infof("%s");

let warn = warnf("%s");

let error = errorf("%s");

let fatal = fatalf("%s");

let isDev = true;
let isDebug = true;

let setup = () => {
  let rotatingFile =
    Log.Output.rotating_file(
      `Text,
      ~basename="onegraph",
      Log.Rotation.create(
        ~size=Byte_units.of_megabytes(100.0),
        ~keep=`At_least(3),
        ~naming_scheme=`Numbered,
        (),
      ),
    );
  Log.Global.set_output(
    List.concat([[rotatingFile], isDev ? [Log.Output.stdout()] : []]),
  );
  Log.Global.set_level(isDebug ? `Debug : `Info);
  info("Logger is set up");
  debug("Logging at debug level");
};
