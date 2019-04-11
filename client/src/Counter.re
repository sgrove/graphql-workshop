let ste = ReasonReact.string;

module GetAllUsers = [%graphql
  {|
query listQuestions {
  question(order_by: {created_at: desc}) {
    id
    body
    created_at
    votes {
      cc_user {
        name
        id
      }
    }
  }
}|}
];

module GetAllUsersQuery = ReasonApollo.CreateSubscription(GetAllUsers);

let component = ReasonReact.statelessComponent("Questions");

let make = _children => {
  ...component,
  render: _self =>
    <GetAllUsersQuery>
      ...{
           ({result}) =>
             <div>
               <h1> {"Questions: " |> ste} </h1>
               {
                 switch (result) {
                 | Error(e) =>
                   Js.log(e);
                   "Something Went Wrong" |> ste;
                 | Loading => "Loading" |> ste
                 | Data(response) =>
                   <div>
                     {
                       response##question
                       |> Array.mapi((index, question) => {
                            let id = question##id;
                            <div key={index |> string_of_int}>
                              {question##body |> ste}
                              <br />
                              <p>
                                {
                                  "ID: "
                                  ++ Js.Json.stringifyAny(id)
                                     ->Belt.Option.getWithDefault("No id")
                                  |> ste
                                }
                              </p>
                            </div>;
                          })
                       |> ReasonReact.array
                     }
                   </div>
                 }
               }
             </div>
         }
    </GetAllUsersQuery>,
};
