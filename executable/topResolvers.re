open Graphql_async;
open GraphQLUtil;

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

let builtInMutations = () =>
  Schema.[
    field(
      "testMutate",
      ~typ=non_null(bool),
      ~args=Arg.[arg("testInput", ~typ=non_null(string))],
      ~resolve=(_: rpContext, (), query) =>
      query == "hi"
    ),
  ];

let makeMutation = (~deprecated=Schema.NotDeprecated, fieldName, typ, args) =>
  Schema.(
    field(
      fieldName, ~typ, ~args, ~deprecated, ~resolve=({ctx, _}: rpContext, ()) =>
      ctx.readOnly
        ? raise(Failure("Refusing to run mutations in read-only mode")) : ()
    )
  );

let mutations = showBeta => [
  ShortenUrlMutation.mutation(),
  ...builtInMutations(showBeta),
];

let schema =
  Schema.(
    schema(
      ~mutations=mutations(),
      ~mutation_name="Mutation",
      ~query_name="Query",
      [
        field(
          "users",
          ~typ=non_null(list(non_null(user))),
          ~args=Arg.[arg("count", ~typ=non_null(int))],
          ~resolve=({ctx, _}: rpContext, (), count) =>
          switch (ctx.isAdmin) {
          | false => List.init(count, _ => ())
          | true => []
          }
        ),
      ],
    )
  );

let makeSchema = _context => {
  schema;
};
