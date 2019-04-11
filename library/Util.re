let genUuid = () => Uuidm.v4_gen(OneRandom.seed, ());

let genUuidString = () => genUuid() |> Uuidm.to_string;

/* Renamed from http://www.codecodex.com/wiki/Generate_a_random_password_or_random_string#OCaml */
let randomString = length => {
  let gen = () =>
    switch (OneRandom.int(26 + 26 + 10)) {
    | n when n < 26 => int_of_char('a') + n
    | n when n < 26 + 26 => int_of_char('A') + n - 26
    | n => int_of_char('0') + n - 26 - 26
    };
  let gen = _ => String.make(1, char_of_int(gen()));
  String.concat("", Array.to_list(Array.init(length, gen)));
};
