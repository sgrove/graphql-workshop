open Graphql_async;

type schemaContext = unit;

type rpContext = Schema.resolve_info(schemaContext);

let user =
  Schema.(
    obj("user", ~doc="A user in the system", ~fields=_ =>
      [
        field(
          "id",
          ~doc="Unique user identifier",
          ~typ=non_null(string),
          ~args=Arg.[],
          ~resolve=(_info: rpContext, ()) =>
          "abc"
        ),
      ]
    )
  );

let schema =
  Schema.(
    schema([
      field(
        "users",
        ~typ=non_null(list(non_null(user))),
        ~args=Arg.[arg("count", ~typ=non_null(int))],
        ~resolve=(_ctx: rpContext, (), count) =>
        List.init(count, _ => ())
      ),
    ])
  );
