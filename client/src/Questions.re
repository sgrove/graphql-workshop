let ste = ReasonReact.string;

module ListQuestions = [%graphql
  {|
subscription listQuestions {
  question(order_by: {created_at: desc}) {
    id
    body
    created_at
    votes_aggregate {
      aggregate {
        count(columns: created_at, distinct: true)
      }
    }
  }
}|}
];

module ListQuestionsQuery = ReasonApollo.CreateSubscription(ListQuestions);

let component = ReasonReact.statelessComponent("Questions");

let questionScore = question =>
  Belt.Option.getWithDefault(
    [%get_in question##votes_aggregate##aggregate#??count],
    0,
  );

let make = _children => {
  ...component,
  render: _self =>
    <ListQuestionsQuery>
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
                       |> Array.to_list
                       |> List.sort((a, b) =>
                            questionScore(b) - questionScore(a)
                          )
                       |> List.mapi((index, question) => {
                            let id = question##id;
                            <div key={index |> string_of_int}>
                              <QuestionVoter questionId=id />
                              <span>
                                {
                                  string_of_int(index + 1)
                                  ++ ". "
                                  ++
                                  question##body
                                  |> ste
                                }
                              </span>
                              <span>
                                <small>
                                  {
                                    "  ["
                                    ++ string_of_int(
                                         questionScore(question),
                                       )
                                    ++ {j| points|j}
                                    ++ "]"
                                    |> ste
                                  }
                                </small>
                              </span>
                            </div>;
                          })
                       |> Array.of_list
                       |> ReasonReact.array
                     }
                   </div>
                 }
               }
             </div>
         }
    </ListQuestionsQuery>,
};
