let genUuid = () => Uuidm.v4_gen(OneRandom.seed, ());

let genUuidString = () => genUuid() |> Uuidm.to_string;
