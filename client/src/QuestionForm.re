let ste = ReasonReact.string;

module AskQuestion = [%graphql
  {|
mutation askQuestion ($body: String!) {
  insert_question(objects: {body: $body}) {
    returning {
      body
      created_at
      id
      updated_at
      votes_aggregate {
        aggregate {
          count(columns: user_id, distinct: true)
        }
      }
    }
  }
}|}
];

module AskQuestionMutation = ReasonApollo.CreateMutation(AskQuestion);

type state = {text: string};

type action =
  | UpdateFormValue(string)
  | Clear;

let component = ReasonReact.reducerComponent("QuestionForm");

let make = _children => {
  ...component,
  initialState: _ => {text: ""},
  reducer: (action, _state) =>
    switch (action) {
    | UpdateFormValue(value) => ReasonReact.Update({text: value})
    | Clear => ReasonReact.Update({text: ""})
    },
  render: self =>
    <AskQuestionMutation>
      ...{
           (mutation, _result) =>
             <div>
               <h1> {"Your Question" |> ste} </h1>
               <h2> {self.state.text |> ste} </h2>
               <textarea
                 value={self.state.text}
                 onChange={
                   event => {
                     let value = ReactEvent.Form.target(event)##value;
                     self.send(UpdateFormValue(value));
                   }
                 }
               />
               <br />
               <button
                 onClick={
                   _mouseEvent => {
                     let askNewQuestion =
                       AskQuestion.make(~body=self.state.text, ());
                     mutation(
                       ~variables=askNewQuestion##variables,
                       ~refetchQueries=[|"listQuestions"|],
                       (),
                     )
                     |> ignore;
                     self.send(Clear);
                   }
                 }>
                 {"Ask away!" |> ste}
               </button>
             </div>
         }
    </AskQuestionMutation>,
};
