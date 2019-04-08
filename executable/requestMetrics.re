module ReqIdWeakMap = OneWeakMap.ReqIdWeakMap;

[@deriving yojson]
type dbCall = unit; /* unimplemented */

[@deriving yojson]
type metrics = {
  dbCalls: list(dbCall),
  apiCalls: list(unit),
  apiCallsSaved: int,
};

let emptyMetrics = () => {apiCalls: [], dbCalls: [], apiCallsSaved: 0};

let reqMap = ReqIdWeakMap.create(32);

let reqStats = reqId => ReqIdWeakMap.find_opt(reqMap, reqId);

[@deriving yojson]
type aggApiInfo = {
  requestCount: int,
  totalRequestMs: float,
};

let aggHostsInfo = (apiCalls: list(OneReqModel.request)) =>
  List.fold_left(
    (acc: list((string, aggApiInfo)), apiCall: OneReqModel.request) =>
      switch (List.assoc_opt(apiCall.host, acc)) {
      | None =>
        List.cons(
          (
            apiCall.host,
            {
              requestCount: 1,
              totalRequestMs: floor(Ooption.default(0.0, apiCall.requestMs)),
            },
          ),
          acc,
        )
      | Some(aggApiInfo) =>
        List.remove_assoc(apiCall.host, acc)
        |> List.cons((
             apiCall.host,
             {
               requestCount: aggApiInfo.requestCount + 1,
               totalRequestMs:
                 aggApiInfo.totalRequestMs
                 +. floor(Ooption.default(0.0, apiCall.requestMs)),
             },
           ))
      },
    [],
    apiCalls,
  );

let totalRequestMs = (apiCalls: list(OneReqModel.request)) =>
  List.fold_left(
    (acc: float, apiCall: OneReqModel.request) =>
      acc +. floor(Ooption.default(0.0, apiCall.requestMs)),
    0.0,
    apiCalls,
  );

let reqExtensionData = (reqId): [< | `Assoc('a)] =>
  switch (reqStats(reqId)) {
  | None => `Assoc([])
  | Some(stats) =>
    `Assoc([
      (
        "metrics",
        `Assoc([
          (
            "api",
            `Assoc([
              ("avoidedRequestCount", `Int(stats.apiCallsSaved)),
              ("requestCount", `Int(List.length(stats.apiCalls))),
              ("totalRequestMs", `Float(totalRequestMs(stats.apiCalls))),
              (
                "byHost",
                `Assoc(
                  List.map(
                    ((host, hostInfo)) =>
                      (
                        host,
                        Yojson.Safe.to_basic(aggApiInfo_to_yojson(hostInfo)),
                      ),
                    aggHostsInfo(stats.apiCalls),
                  ),
                ),
              ),
            ]),
          ),
        ]),
      ),
    ])
  };

let reqStatsString = reqId =>
  switch (reqStats(reqId)) {
  | None => ""
  | Some(stats) =>
    Printf.sprintf(
      "api_req_count=%d, metricsLength=%d",
      List.length(stats.apiCalls),
      ReqIdWeakMap.length(reqMap),
    )
  };

let addApiCall = (metrics, apiRequest) => {
  ...metrics,
  apiCalls: List.cons(apiRequest, metrics.apiCalls),
};

let recordDbCall = reqId =>
  try (
    switch (ReqIdWeakMap.find_opt(reqMap, reqId)) {
    | None => ReqIdWeakMap.add(reqMap, reqId, emptyMetrics())
    | Some(_metrics) => ReqIdWeakMap.replace(reqMap, reqId, emptyMetrics())
    }
  ) {
  | exn =>
    OneLog.errorf(
      "Error recording api call exn=%sa",
      Core.Exn.to_string(exn),
    )
  };

let recordApiCall = (reqId, apiCall: OneReqModel.request) =>
  try (
    switch (ReqIdWeakMap.find_opt(reqMap, reqId)) {
    | None =>
      ReqIdWeakMap.add(reqMap, reqId, addApiCall(emptyMetrics(), apiCall))
    | Some(metrics) =>
      ReqIdWeakMap.replace(reqMap, reqId, addApiCall(metrics, apiCall))
    }
  ) {
  | exn =>
    OneLog.errorf("Error recording api call exn=%s", Core.Exn.to_string(exn))
  };

let recordSavedApiCalls = (reqId, count: int) =>
  try (
    switch (ReqIdWeakMap.find_opt(reqMap, reqId)) {
    | None =>
      ReqIdWeakMap.add(
        reqMap,
        reqId,
        {...emptyMetrics(), apiCallsSaved: count},
      )
    | Some(metrics) =>
      ReqIdWeakMap.replace(
        reqMap,
        reqId,
        {...metrics, apiCallsSaved: metrics.apiCallsSaved + count},
      )
    }
  ) {
  | exn =>
    OneLog.errorf(
      "Error recording api call saved exn=%s",
      Core.Exn.to_string(exn),
    )
  };
