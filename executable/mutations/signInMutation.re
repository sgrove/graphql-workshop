open Graphql_async;
open GraphQLUtil;

module Mutation = {
  let make = meGqlType => {
    /* This isn't used yet, but it should be exposed on the OneGraph
       dash somehow so an end-user can log out of all OG apps if they
       want to. When this is uncommented, it should follow the
       mutation guidelines
       https://paper.dropbox.com/doc/GraphQL-Consistency-wJXftnfs722Vhl1dLgu8d
       to be consistent */
    let rec topMutations =
      lazy
        Schema.(
          obj(
            "OneGraphMutation",
            ~doc="Mutations for the currently authed user",
            ~fields=_ =>
            [
              ShortenUrlMutation.mutation(
                ~oneGraphShortenedUrlGqlType=shortenedOneGraphQuery,
              ),
              io_field(
                "signUp",
                ~typ=non_null(oneGraphSignInResult),
                ~args=
                  Arg.[
                    arg("fullName", ~typ=non_null(string)),
                    arg("email", ~typ=non_null(string)),
                    arg("password", ~typ=non_null(string)),
                    arg("passwordConfirm", ~typ=non_null(string)),
                    arg("agreeToTOS", ~typ=non_null(bool)),
                  ],
                ~resolve=(
                           {ctx, _}: rpContext,
                           (),
                           fullName,
                           email,
                           password,
                           passwordConfirm,
                           agreeToTOS,
                         ) => {
                  let userId = Utils.genUuid();
                  OneUserModel.insertUser(
                    ~db=ctx.db,
                    ~id=userId,
                    ~fullName,
                    ~email,
                    ~password,
                    ~passwordConfirm,
                    ~confirmed=false,
                    ~agreeToTOS,
                  )
                  >>= (
                    fun
                    | Error(e) =>
                      Deferred.return(Error(dbErrorToResolverError(e)))
                    | Ok () =>
                      Deferred.both(
                        OneUserModel.byEmailAndPassword(
                          ~db=ctx.db,
                          ~email,
                          ~password,
                        ),
                        Deferred.both(
                          AccessObjectModel.createForOneGraphDotComUser(
                            ~dbConn=ctx.db,
                            ~appId=ctx.appId,
                            ~userId,
                          ),
                          OrgModel.createOrg(
                            ~db=ctx.db,
                            ~name=Printf.sprintf("%s's org", fullName),
                            ~role=OrgModel.Owner,
                            ~userId,
                          ),
                        ),
                      )
                      >>| (
                        fun
                        | (Error(_), _)
                        | (_, (_, Error(_)))
                        | (_, (Error(_), _)) =>
                          Error(
                            `Unspecified(
                              "Error creating user. Please try again",
                            ),
                          )
                        | (Ok(None), _) => {
                            OneLog.errorf(
                              "Unable to find user by email/password after inserting user",
                            );
                            Error(
                              `Unspecified(
                                "Error creating user. Please try again.",
                              ),
                            );
                          }
                        | (
                            Ok(Some(user)),
                            (Ok(accessObject), Ok(_orgId)),
                          ) => {
                            slackNewUserSuccess(email, fullName);
                            OA.(event(SignUp(user))) |> ignore;
                            Ok({accessObject: accessObject});
                          }
                      )
                  );
                },
              ),
              io_field(
                "signIn",
                ~typ=non_null(oneGraphSignInResult),
                ~args=
                  Arg.[
                    arg("email", ~typ=non_null(string)),
                    arg("password", ~typ=non_null(string)),
                    arg("rememberMe", ~typ=non_null(bool)),
                  ],
                ~resolve=(
                           {ctx, _}: rpContext,
                           (),
                           email,
                           password,
                           _rememberMe,
                         ) =>
                OneUserModel.byEmailAndPassword(ctx.db, email, password)
                >>= (
                  fun
                  | Error(e) =>
                    Deferred.return(Error(dbErrorToResolverError(e)))
                  | Ok(None) =>
                    Deferred.return(
                      Error(
                        `Unspecified(
                          "We couldn't find a user with that email and password",
                        ),
                      ),
                    )

                  | Ok(Some(user)) =>
                    AccessObjectModel.createForOneGraphDotComUser(
                      ~dbConn=ctx.db,
                      ~appId=ctx.appId,
                      ~userId=user.id,
                    )
                    >>| (
                      fun
                      | Ok(accessObject) => {
                          OA.(
                            event(
                              SignIn({userId: Uuidm.to_string(user.id)}),
                            )
                          )
                          |> ignore;
                          Ok({accessObject: accessObject});
                        }
                      | Error(_) =>
                        Error(
                          `Unspecified(
                            "Internal error when signing in. Please try again.",
                          ),
                        )
                    )
                )
              ),
              io_field(
                "signOut",
                ~typ=non_null(signoutResponsePayload(meGqlType)),
                ~args=Arg.[],
                ~resolve=({ctx, _}: rpContext, ()) =>
                switch (ctx.session^) {
                | None => Async.Deferred.return(Ok())
                | Some(session) =>
                  OneSession.clearOneUserId(ctx.db, session)
                  >>= (
                    fun
                    | Result.Ok(clearedOneUserSession) =>
                      OneSession.clearUserAuthIdsByAppId(
                        ctx.db,
                        clearedOneUserSession,
                        ctx.appId,
                      )
                      >>= (
                        fun
                        | Result.Ok(_clearedSession) =>
                          Async.Deferred.return(Ok())
                        | Result.Error(_error) =>
                          failPublic(~public="Unable to complete signout", ())
                      )
                    | Result.Error(_session) =>
                      failPublic(~public="Unable to complete signout", ())
                  )
                }
              ),
              io_field(
                "requestPasswordReset",
                ~typ=non_null(string),
                ~args=Arg.[arg("email", ~typ=non_null(string))],
                ~resolve=({ctx, _}: rpContext, (), email) =>
                OneUserModel.resetPasswordResetTokenByEmail(ctx.db, email)
                >>= (
                  fun
                  | Error(error) => Error(error) |> defer
                  | Ok(None) =>
                    /* NB: Might be a good idea to add a delay on this branch so it can't be used to detect if an email is valid */
                    Async.Deferred.return(Ok("Reset email sent"))
                  | Ok(Some(user)) => {
                      let subject =
                        OneConfig.details.isProd
                          ? Printf.sprintf("Onegraph.com password reset")
                          : Printf.sprintf(
                              "[Dev] OneGraph.dev password reset",
                            );
                      let body =
                        Printf.sprintf(
                          "Reset token: %s",
                          user.resetPasswordToken
                          |> Ooption.default("No reset token available"),
                        );
                      let params = [
                        ("to", email),
                        ("from", "OneGraph Support <support@onegraph.com>"),
                        ("subject", subject),
                        ("text", body),
                      ];
                      ignore(
                        Mailgun.send(
                          ~domain="mg.onegraph.com",
                          ~apiKey=OneConfig.details.mailgunApiKey,
                          params,
                        ),
                      );
                      Async.Deferred.return(Ok("Reset email sent"));
                    }
                )
                >>| handleResult
              ),
              io_field(
                "resetPassword",
                ~typ=non_null(bool),
                ~args=
                  Arg.[
                    arg("token", ~typ=non_null(string)),
                    arg("password", ~typ=non_null(string)),
                    arg("passwordConfirm", ~typ=non_null(string)),
                  ],
                ~resolve=(
                           {ctx, _}: rpContext,
                           (),
                           token,
                           password,
                           passwordConfirm,
                         ) =>
                OneUserModel.resetPasswordByResetPasswordToken(
                  ctx.db,
                  token,
                  password,
                  passwordConfirm,
                )
                >>| (
                  fun
                  | Ok(_) => Ok(true)
                  | Error(_message) => Ok(false)
                )
              ),
            ]
          )
        );
    topMutations;
  };
};
