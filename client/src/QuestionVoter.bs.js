// Generated by BUCKLESCRIPT VERSION 4.0.18, PLEASE EDIT WITH CARE
'use strict';

var Curry = require("bs-platform/lib/js/curry.js");
var React = require("react");
var Js_exn = require("bs-platform/lib/js/js_exn.js");
var Js_dict = require("bs-platform/lib/js/js_dict.js");
var Js_json = require("bs-platform/lib/js/js_json.js");
var Caml_option = require("bs-platform/lib/js/caml_option.js");
var ReasonReact = require("reason-react/src/ReasonReact.js");
var ReasonApollo = require("reason-apollo/src/ReasonApollo.bs.js");

function ste(prim) {
  return prim;
}

var ppx_printed_query = "mutation voteForQuestion($questionId: uuid)  {\ninsert_vote(objects: {question_id: $questionId})  {\nreturning  {\nquestion  {\nvotes_aggregate  {\naggregate  {\ncount(columns: created_at, distinct: true)  \n}\n\n}\n\n}\n\n}\n\n}\n\n}\n";

function parse(value) {
  var match = Js_json.decodeObject(value);
  if (match !== undefined) {
    var match$1 = Js_dict.get(Caml_option.valFromOption(match), "insert_vote");
    var tmp;
    if (match$1 !== undefined) {
      var value$1 = Caml_option.valFromOption(match$1);
      var match$2 = Js_json.decodeNull(value$1);
      if (match$2 !== undefined) {
        tmp = undefined;
      } else {
        var match$3 = Js_json.decodeObject(value$1);
        var tmp$1;
        if (match$3 !== undefined) {
          var match$4 = Js_dict.get(Caml_option.valFromOption(match$3), "returning");
          var tmp$2;
          if (match$4 !== undefined) {
            var value$2 = Caml_option.valFromOption(match$4);
            var match$5 = Js_json.decodeArray(value$2);
            tmp$2 = match$5 !== undefined ? match$5.map((function (value) {
                      var match = Js_json.decodeObject(value);
                      if (match !== undefined) {
                        var match$1 = Js_dict.get(Caml_option.valFromOption(match), "question");
                        var tmp;
                        if (match$1 !== undefined) {
                          var match$2 = Js_json.decodeObject(Caml_option.valFromOption(match$1));
                          if (match$2 !== undefined) {
                            var match$3 = Js_dict.get(Caml_option.valFromOption(match$2), "votes_aggregate");
                            var tmp$1;
                            if (match$3 !== undefined) {
                              var match$4 = Js_json.decodeObject(Caml_option.valFromOption(match$3));
                              if (match$4 !== undefined) {
                                var match$5 = Js_dict.get(Caml_option.valFromOption(match$4), "aggregate");
                                var tmp$2;
                                if (match$5 !== undefined) {
                                  var value$1 = Caml_option.valFromOption(match$5);
                                  var match$6 = Js_json.decodeNull(value$1);
                                  if (match$6 !== undefined) {
                                    tmp$2 = undefined;
                                  } else {
                                    var match$7 = Js_json.decodeObject(value$1);
                                    var tmp$3;
                                    if (match$7 !== undefined) {
                                      var match$8 = Js_dict.get(Caml_option.valFromOption(match$7), "count");
                                      var tmp$4;
                                      if (match$8 !== undefined) {
                                        var value$2 = Caml_option.valFromOption(match$8);
                                        var match$9 = Js_json.decodeNull(value$2);
                                        if (match$9 !== undefined) {
                                          tmp$4 = undefined;
                                        } else {
                                          var match$10 = Js_json.decodeNumber(value$2);
                                          tmp$4 = match$10 !== undefined ? match$10 | 0 : Js_exn.raiseError("graphql_ppx: Expected int, got " + JSON.stringify(value$2));
                                        }
                                      } else {
                                        tmp$4 = undefined;
                                      }
                                      tmp$3 = {
                                        count: tmp$4
                                      };
                                    } else {
                                      tmp$3 = Js_exn.raiseError("graphql_ppx: Object is not a value");
                                    }
                                    tmp$2 = Caml_option.some(tmp$3);
                                  }
                                } else {
                                  tmp$2 = undefined;
                                }
                                tmp$1 = {
                                  aggregate: tmp$2
                                };
                              } else {
                                tmp$1 = Js_exn.raiseError("graphql_ppx: Object is not a value");
                              }
                            } else {
                              tmp$1 = Js_exn.raiseError("graphql_ppx: Field votes_aggregate on type question is missing");
                            }
                            tmp = {
                              votes_aggregate: tmp$1
                            };
                          } else {
                            tmp = Js_exn.raiseError("graphql_ppx: Object is not a value");
                          }
                        } else {
                          tmp = Js_exn.raiseError("graphql_ppx: Field question on type vote is missing");
                        }
                        return {
                                question: tmp
                              };
                      } else {
                        return Js_exn.raiseError("graphql_ppx: Object is not a value");
                      }
                    })) : Js_exn.raiseError("graphql_ppx: Expected array, got " + JSON.stringify(value$2));
          } else {
            tmp$2 = Js_exn.raiseError("graphql_ppx: Field returning on type vote_mutation_response is missing");
          }
          tmp$1 = {
            returning: tmp$2
          };
        } else {
          tmp$1 = Js_exn.raiseError("graphql_ppx: Object is not a value");
        }
        tmp = Caml_option.some(tmp$1);
      }
    } else {
      tmp = undefined;
    }
    return {
            insert_vote: tmp
          };
  } else {
    return Js_exn.raiseError("graphql_ppx: Object is not a value");
  }
}

function make(questionId, param) {
  return {
          query: ppx_printed_query,
          variables: Js_dict.fromArray(/* array */[/* tuple */[
                  "questionId",
                  questionId !== undefined ? Caml_option.valFromOption(questionId) : null
                ]]),
          parse: parse
        };
}

function makeWithVariables(variables) {
  var questionId = variables.questionId;
  return {
          query: ppx_printed_query,
          variables: Js_dict.fromArray(/* array */[/* tuple */[
                  "questionId",
                  questionId !== undefined ? Caml_option.valFromOption(questionId) : null
                ]]),
          parse: parse
        };
}

function ret_type(f) {
  return /* module */[];
}

var MT_Ret = /* module */[];

var VoteForQuestion = /* module */[
  /* ppx_printed_query */ppx_printed_query,
  /* query */ppx_printed_query,
  /* parse */parse,
  /* make */make,
  /* makeWithVariables */makeWithVariables,
  /* ret_type */ret_type,
  /* MT_Ret */MT_Ret
];

var VoteForQuestionMutation = ReasonApollo.CreateMutation([
      ppx_printed_query,
      parse
    ]);

var component = ReasonReact.statelessComponent("QuestionVoter");

function make$1(questionId, _children) {
  return /* record */[
          /* debugName */component[/* debugName */0],
          /* reactClassInternal */component[/* reactClassInternal */1],
          /* handedOffState */component[/* handedOffState */2],
          /* willReceiveProps */component[/* willReceiveProps */3],
          /* didMount */component[/* didMount */4],
          /* didUpdate */component[/* didUpdate */5],
          /* willUnmount */component[/* willUnmount */6],
          /* willUpdate */component[/* willUpdate */7],
          /* shouldUpdate */component[/* shouldUpdate */8],
          /* render */(function (_self) {
              return ReasonReact.element(undefined, undefined, Curry._4(VoteForQuestionMutation[/* make */5], undefined, undefined, undefined, (function (mutation, _result) {
                                return React.createElement("button", {
                                            onClick: (function (_mouseEvent) {
                                                var askNewQuestion = make(Caml_option.some(questionId), /* () */0);
                                                Curry._4(mutation, Caml_option.some(askNewQuestion.variables), /* array */["listQuestions"], undefined, /* () */0);
                                                return /* () */0;
                                              })
                                          }, "↑");
                              })));
            }),
          /* initialState */component[/* initialState */10],
          /* retainedProps */component[/* retainedProps */11],
          /* reducer */component[/* reducer */12],
          /* jsElementWrapped */component[/* jsElementWrapped */13]
        ];
}

exports.ste = ste;
exports.VoteForQuestion = VoteForQuestion;
exports.VoteForQuestionMutation = VoteForQuestionMutation;
exports.component = component;
exports.make = make$1;
/* VoteForQuestionMutation Not a pure module */