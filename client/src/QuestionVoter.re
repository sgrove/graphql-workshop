let ste = ReasonReact.string;

module VoteForQuestion = [%graphql
  {|
mutation voteForQuestion ($questionId: uuid) {
  insert_vote(objects: {question_id: $questionId}) {
    returning {
      question {
        votes_aggregate {
          aggregate {
            count(columns: created_at, distinct: true)
          }
        }
      }
    }
  }
}|}
];

module VoteForQuestionMutation = ReasonApollo.CreateMutation(VoteForQuestion);

type state = {text: string};

let component = ReasonReact.statelessComponent("QuestionVoter");

let make = (~questionId, _children) => {
  ...component,
  /* State transitions */
  render: _self =>
    <VoteForQuestionMutation>
      ...{
           (mutation, _result) =>
             <button
               onClick={
                 _mouseEvent => {
                   let askNewQuestion = VoteForQuestion.make(~questionId, ());
                   mutation(
                     ~variables=askNewQuestion##variables,
                     ~refetchQueries=[|"listQuestions"|],
                     (),
                   )
                   |> ignore;
                 }
               }>
               {{j|↑|j} |> ste}
             </button>
         }
    </VoteForQuestionMutation>,
};
