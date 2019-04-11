/* Create an InMemoryCache */
let inMemoryCache = ApolloInMemoryCache.createInMemoryCache();

let authLink =
  ApolloLinks.createContextLink(() =>
    {
      "headers": {
        "x-hasura-admin-secret": {j|myadminsecretkey|j},
      },
    }
  );

/* Create an HTTP Link */
let httpLink =
  ApolloLinks.createHttpLink(
    ~uri="http://localhost:8080/v1alpha1/graphql",
    ~credentials="include",
    (),
  );

let webSocketLink =
  ApolloLinks.webSocketLink(
    ~uri="ws://localhost:8080/v1alpha1/graphql",
    ~reconnect=true,
    (),
  );

/* based on test, execute left or right */
let webSocketHttpLink =
  ApolloLinks.split(
    operation => {
      let operationDefition =
        ApolloUtilities.getMainDefinition(operation##query);
      operationDefition##kind == "OperationDefinition"
      &&
      operationDefition##operation == "subscription";
    },
    webSocketLink,
    httpLink,
  );

let instance =
  ReasonApollo.createApolloClient(
    ~link=ApolloLinks.from([|authLink, webSocketHttpLink|]),
    ~cache=inMemoryCache,
    (),
  );
