let default = (value, opt) =>
  switch (opt) {
  | None => value
  | Some(value) => value
  };

let expect = (name, opt) =>
  switch (opt) {
  | None => raise(Failure("Missing value for " ++ name))
  | Some(value) => value
  };

let map = (opt, update) =>
  switch (opt) {
  | None => None
  | Some(value) => Some(update(value))
  };

let fmap = (opt, update) =>
  switch (opt) {
  | None => None
  | Some(value) => update(value)
  };

let fmapDefault = (default, opt, update) =>
  switch (opt) {
  | None => default
  | Some(value) => update(value)
  };

let effectMap = (opt, f) =>
  switch (opt) {
  | None => ()
  | Some(value) => f(value)
  };
