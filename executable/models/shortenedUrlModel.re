open Async;

module ById = [%graphql
  {|
query shortenedUrlById ($id: String!){
  shortened_url_by_pk(id: $id) {
    hits
    enabled
    id
    url
    created_at
    updated_at
  }
}|}
];

let byId = (~authSecret, ~id: string) => {
  let query = ById.make(~id, ());

  GraphQLClient.(hasura(~authSecret, query))
  >>| (result => GraphQLUtil.handleErr(result))
  >>| (result => result#shortened_url_by_pk);
};

module ByIdWithHit = [%graphql
  {|
mutation shortenedUrlByIdWithHit($id: String!) {
  update_shortened_url(where: {id: {_eq: $id}}, _inc: {hits: 1}) {
    returning {
      hits
      id
      enabled
      url
      updated_at
      created_at
    }
  }
}|}
];

let byIdWithHit = (~authSecret, ~id: string) => {
  let mutation = ByIdWithHit.make(~id, ());

  GraphQLClient.(hasura(~authSecret, mutation))
  >>| (result => GraphQLUtil.handleErr(result))
  >>| (
    result =>
      result#update_shortened_url
      |> Workshop.Ooption.map(_, item => item#returning[0])
  );
};

module Insert = [%graphql
  {|
mutation makeShortenedUrl($url: String!, $id: String!){
  insert_shortened_url(objects: [{id: $id, url: $url, enabled: true}]) {
    returning {
      id
      enabled
    }
  }
}|}
];

let insert = (~authSecret, ~url: Uri.t, ~id: string) => {
  let mutation = Insert.make(~url=Uri.to_string(url), ~id, ());

  GraphQLClient.(hasura(~authSecret, mutation))
  >>| (result => GraphQLUtil.handleErr(result))
  >>| (
    result =>
      switch (result#insert_shortened_url) {
      | None => None
      | Some(url) => Some(url#returning[0])
      }
  );
};
