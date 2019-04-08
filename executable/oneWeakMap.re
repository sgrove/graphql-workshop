module UuidHashable = {
  type t = Uuidm.t;
  let hash = Hashtbl.hash;
  let equal = (x, y) => x == y;
};

module ReqIdWeakMap = Ephemeron.K1.Make(UuidHashable);
