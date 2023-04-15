##NAME##

explain - get the access plan for the query

##SYNOPSIS##

**query.explain([options])**

##CATEGORY##

SdbQuery

##DESCRIPTION##

This function is used to get the access plan for the query.

##PARAMETERS##

options ( *object, optional* )

The output information of the access plan can be controlled through the parameter "options":

- Run ( *boolean* ): Whether to run the access plan, and the default value is "false".

    The values are as follows:

    - true: Run access plan and output access plan information.
    - false: Only output access plan information, do not run.

    Format: `Run: true`

- Detail ( *boolean* ): Whether to output detailed access plan, and the default value is "false".

    When the value of parameter "Detail" is "true", a layer of detailed access plan is displayed by default.

    The values are as follows:

    - true: Displayed [detailed access plan][explain_det].
    - false: Displayed [normal access plan][explain_ord].

    Format: `Detail: true`

- Estimate ( *boolean* ): Whether to display the estimate part of the detailed access plan, and the default value is the value of the parameter "Detail".

    If parameter "Estimate" is explicitly set, parameter "Detail" will be automatically set to "true". 

    The values are as follows:

    - true: Display the estimated part.
    - false：Do not display the estimated part.

    Format: `Estimate: true`

- Expand ( *boolean* ): Whether to display the expanded information of the detailed access plan, and the default value is "false", which means not to display.

    If parameter "Expand" is explicitly set, parameter "Detail" will be automatically set to "true". 

    Format: `Expand: true`

- Flatten ( *boolean* ): Whether to display the access plan of each node and each sub-collection separately, and the default value is "false".

    If parameter "Flatten" is explicitly set, parameter "Detail" and "Expand" will be automatically set to "true". 

    The values are as follows:

    - true: Display the access plan of each node and each sub-collection separately.
    - false: Combine the access plans of nodes and sub-collections into an array and hang them on the upper-level node or main collection for display.

    Format: `Flatten: true`

- Filter ( *string/array* ): Filter the details of the estimated results, and the default value is "ALL".

    If parameter "Filter" is explicitly set, parameter "Detail" and "Estimate" will be automatically set to "true". 

    The values are as follows:

    - "None": Do not display any details of the estimated results. 
    - "Input": Display input details of the estimated results.
    - "Filter": Display filtering details of the estimated results.
    - "Output": Display output details of the estimated results.
    - "All": Display all details of the estimated results.

    Format: `Filter: ["Input", "Output"]`

- CMDLocation ( "object" ): Filter the results of the access plan according to the data groups, and the default is null, which means that no filter condition is set.

    - Parameter "CMDLocation" only supports filtering by replication group ID (corresponding to parameter "GroupID") and replication group name (corresponding to parameter "GroupName").
    - If parameter "CMDLocation" is explicitly set, parameter "Detail" will be automatically set to "true". 

    Format: `CMDLocation: {GroupName: "group1"}`

- SubCollections ( *string/array* ): Filter the results of the access plan according to one or multiple sub-collections, and the default is null, which means that no filter condition is set.

    - This parameter only takes effect in the access plan involving the main collection and sub-collection.
    - When specifying this parameter, it is necessary to specify the parameter "Expand" as "true" at the same time. 

    Format: `SubCollections: ["subcs.subcl1", "subcs.subcl2"]`

- Search ( *boolean* ): Whether to display the [search process of the access plan][cost_estimation], and the default value is "false", which means not to display.

    If parameter "Search" is explicitly set, parameter "Detail" and "Expand" will be automatically set to "true". 

    Format: `Search: true`

- Evaluate ( *boolean* ): Whether to display the deduction formula of the query optimizer, and the default value is "false", which means not to display.

    If parameter "Evaluate" is explicitly set, parameter "Detail", "Search" and "Expand" will be automatically set to "true". 

    Format: `Evaluate: true`

- Abbrev ( *boolean* ): Whether to output long strings in abbreviation mode, and the default value is "false", which means not to abbreviate the output.

    Format: `Abbrev: true`

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get the access plan of the query through this object, refer to [access plan][explain].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Get the normal access plan for a query.

    ```lang-javascript
    > db.sample.employee.find({a:{$gte:100}}).explain()
    {
      "NodeName": "hostname:11820",
      "GroupName": "group1",
      "Role": "data",
      "Name": "sample.employee",
      "ScanType": "tbscan",
      "IndexName": "",
      "UseExtSort": false,
      "Query": {
        "$and": [
          {
            "a": {
              "$gte": 100
            }
          }
        ]
      },
      "IXBound": null,
      "NeedMatch": true,
      "IndexCover": false,
      "ReturnNum": 49892,
      "ElapsedTime": 0.323423,
      "IndexRead": 0,
      "DataRead": 49945,
      "UserCPU": 0.1399999999999999,
      "SysCPU": 0
    }
    ...
    ```

- Get the detailed access plan for a query.

    ```lang-javascript
    > db.sample.employee.find({a: {$gt: 100}}).explain({Detail: true})
    {
      "NodeName": "hostname:11810",
      "GroupName": "SYSCoord",
      "Role": "coord",
      "Collection": "sample.employee",
      "Query": {
        "a": {
          "$gt": 100
        }
      },
      "Sort": {},
      "Selector": {},
      "Hint": {},
      "Skip": 0,
      "Return": -1,
      "Flag": 0,
      "ReturnNum": 0,
      "ElapsedTime": 0.00123,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0,
      "SysCPU": 0,
      "PlanPath": {
        "Operator": "COORD-MERGE",
        "Sort": {},
        "NeedReorder": false,
        "DataNodeNum": 2,
        "DataNodeList": [
          {
            "Name": "hostname:11820",
            "EstTotalCost": 1.484
          },
          {
            "Name": "hostname:11830",
            "EstTotalCost": 0.7418349999999999
          }
        ],
        "Selector": {},
        "Skip": 0,
        "Return": -1,
        "Estimate": {
          "StartCost": 0,
          "RunCost": 1.5214865,
          "TotalCost": 1.5214865,
          "Output": {
            "Records": 74973,
            "RecordSize": 29,
            "Sorted": false
          }
        },
        "ChildOperators": [
          {
            "NodeName": "hostname:11820",
            "GroupName": "group1",
            "Role": "data",
            "Collection": "sample.employee",
            "Query": {
              "a": {
                "$gt": 100
              }
            },
            "Sort": {},
            "Selector": {},
            "Hint": {},
            "Skip": 0,
            "Return": -1,
            "Flag": 2048,
            "ReturnNum": 0,
            "ElapsedTime": 0.000078,
            "IndexRead": 0,
            "DataRead": 0,
            "UserCPU": 0,
            "SysCPU": 0,
            "CacheStatus": "HitCache",
            "MainCLPlan": false,
            "CacheLevel": "OPT_PLAN_PARAMETERIZED",
            "Parameters": [
              100
            ],
            "MatchConfig": {
              "EnableMixCmp": false,
              "Parameterized": true,
              "FuzzyOptr": false
            }
          },
          {
            "NodeName": "hostname:11830",
            "GroupName": "group2",
            "Role": "data",
            "Collection": "sample.employee",
            "Query": {
              "a": {
                "$gt": 100
              }
            },
            "Sort": {},
            "Selector": {},
            "Hint": {},
            "Skip": 0,
            "Return": -1,
            "Flag": 2048,
            "ReturnNum": 0,
            "ElapsedTime": 0.000081,
            "IndexRead": 0,
            "DataRead": 0,
            "UserCPU": 0,
            "SysCPU": 0,
            "CacheStatus": "HitCache",
            "MainCLPlan": false,
            "CacheLevel": "OPT_PLAN_PARAMETERIZED",
            "Parameters": [
              100
            ],
            "MatchConfig": {
              "EnableMixCmp": false,
              "Parameterized": true,
              "FuzzyOptr": false
            }
          }
        ]
      }
    }
    ```

- Specify the parameter "Run" as "true", run and get the detailed access plan for a query.

    ```lang-javascript
    > db.sample.employee.find({a: {$gt: 100}}).explain({Run: true, Detail: true})
    ...
        "Run": {
          "ContextID": 29314,
          "StartTimestamp": "2017-12-14-15.24.51.254623",
          "QueryTimeSpent": 0.821182,
          "GetMores": 112,
          "ReturnNum": 99899,
          "WaitTimeSpent": 0.075
        },
    ...
    ```

- Specify the parameter "Expand" as "true" to get the expanded information in the detailed access plan.

    ```lang-javascript
    > db.sample.employee.find({a: {$gt: 100}}).explain({Expand: true})
    ...
    "PlanPath": {
      "Operator": "TBSCAN",
      "Collection": "sample.employee",
      "Query": {
        "$and": [
          {
            "a": {
              "$gt": 100
            }
          }
        ]
      },
      "Selector": {},
      "Skip": 0,
      "Return": -1,
      "Estimate": {
        "StartCost": 0,
        "RunCost": 0.0007999999999999999,
        "TotalCost": 0.0007999999999999999,
        "CLEstFromStat": false,
        "Input": {
          "Pages": 1,
          "Records": 200,
          "RecordSize": 1
        },
        "Filter": {
          "MthSelectivity": 0.49999995
        },
        "Output": {
          "Records": 100,
          "RecordSize": 1,
          "Sorted": false
        }
      }
    }
    ...
    ```

- Specify the parameter "Search" as "true" to get the search process of the detailed access plan.

    ```lang-javascript
    > db.sample.employee.find({a: {$gt: 100}}).explain(Search: true})
    ...  
    "Search": {
      "Options": {
        "sortbuf": 256,
        "optcostthreshold": 20
      },
      "SearchPaths": [
        {
          "IsUsed": false,
          "IsCandidate": false,
          "Score": 1,
          "ScanType": "ixscan",
          "IndexName": "$id",
          "UseExtSort": false,
          "Direction": 1,
          "IXBound": {
            "_id": [
              [
                {
                  "$minElement": 1
                },
                {
                  "$maxElement": 1
                }
              ]
            ]
          },
          "NeedMatch": true,
          "IndexCover": false,
          "IXEstFromStat": false
        },
        {
          "IsUsed": false,
          "IsCandidate": false,
          "Score": 0.4999994999999995,
          "ScanType": "ixscan",
          "IndexName": "$shard",
          "UseExtSort": false,
          "Direction": 1,
          "IXBound": {
            "a": [
              [
                100,
                {
                  "$decimal": "MAX"
                }
              ]
            ]
          },
          "NeedMatch": false,
          "IndexCover": false,
          "IXEstFromStat": false
        },
        {
          "IsUsed": true,
          "IsCandidate": true,
          "Score": 0.4999994999999995,
          "TotalCost": 1483670,
          "ScanType": "tbscan",
          "IndexName": "",
          "UseExtSort": false
        }
      ]
    }
    ...
    ```

- Specify the parameter "CMDLocation" to get the detailed access plan of the query on the replication group "group1".

    ```lang-javascript
    > db.sample.employee.find({a: {$gt: 100}}).explain({Detail: true, CMDLocation: {GroupName: "group1"}})
    {
    ...
    "ChildOperators": [
      {
        "NodeName": "hostname:11810",
        "GroupName": "group1",
        "Role": "data",
        "Collection": "sample.employee",
        "Query": {
          "a": {
            "$gt": 100
          }
        },
        "Sort": {},
        "Selector": {},
        "Hint": {},
        "Skip": 0,
        "Return": -1,
        "Flag": 2048,
        "ReturnNum": 0,
        "ElapsedTime": 0.000088,
        "IndexRead": 0,
        "DataRead": 0,
        "UserCPU": 0,
        "SysCPU": 0,
        "CacheStatus": "HitCache",
        "MainCLPlan": false,
        "CacheLevel": "OPT_PLAN_PARAMETERIZED",
        "Parameters": [
          100
        ],
        "MatchConfig": {
          "EnableMixCmp": false,
          "Parameterized": true,
          "FuzzyOptr": false
        }
      }
    ]
    ...
    ```

[^_^]:
     Links
[explain]:manual/Distributed_Engine/Maintainance/Access_Plan/explain.md
[cost_estimation]:manual/Distributed_Engine/Maintainance/Access_Plan/cost_estimation.md#访问计划的搜索过程
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[location]:manual/Manual/Sequoiadb_Command/location.md
[explain_det]:manual/Distributed_Engine/Maintainance/Access_Plan/explain.md#详细的访问计划
[explain_ord]:manual/Distributed_Engine/Maintainance/Access_Plan/explain.md#普通访问计划
[Evaluate]:manual/Manual/Cost_Estimation/Readme.md
