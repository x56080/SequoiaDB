##名称##

explain - 获取查询的访问计划

##语法##

**query.explain([options])**

##类别##

SdbQuery

##描述##

该函数用于获取查询的访问计划。

##参数##

options（ *string，选填* ）

通过参数 options 可以控制访问计划的输出信息：

- Run（ *boolean* ）：是否执行访问计划，默认值为 false

    取值如下：

    - true：执行访问计划并展示访问计划的信息
    - false：仅展示访问计划的信息，并不执行

    格式：`Run: true`

- Detail（ *boolean* ）：是否展示详细访问计划，默认值为 false 

    参数 Detail 取值为 true 时，默认展示一层详细访问计划。

    取值如下：

    - true: 展示[详细访问计划][explain_det]
    - false: 展示[普通访问计划][explain_ord]

    格式：`Detail: true`

- Estimate（ *boolean* ）：是否展示详细访问计划中的估算部分，默认值为参数 Detail 的取值

    如果参数 Estimate 显式设置，参数 Detail 将自动设置为 true。

    取值如下：

    - true：展示估算部分
    - false：不展示估算部分

    格式：`Estimate: true`

- Expand（ *boolean* ）：是否展示详细访问计划中的扩展信息，默认值为 false，表示不展示

    如果参数 Expand 显式设置，参数 Detail 将自动设置为 true。

    格式：`Expand: true`

- Flatten（ *boolean* ）：是否分别展示每个节点和每个子集合的访问计划，默认值为 false

    如果参数 Flatten 显式设置，参数 Detail 和 Expand 将自动设置为 true。

    取值如下：

    - true：分别展示每个节点和每个子集合的访问计划
    - false：将节点和子集合的访问计划组合成数组挂在上一级节点或主集合上进行展示

    格式：`Flatten: true`

- Filter（ *string/array* ）：对估算结果的细节进行过滤，默认值为"ALL"

    如果参数 Filter 显式设置，参数 Detail 和 Estimate 将自动设置为 true。

    取值如下：

    - "None"：不展示估算结果的任何细节
    - "Input"：展示估算结果的输入细节
    - "Filter"：展示估算结果的过滤细节
    - "Output"：展示估算结果的输出细节
    - "All"：展示估算结果的全部细节

    格式：`Filter: ["Input", "Output"]`

- CMDLocation（ *object* ）：对访问计划的结果按照数据组进行过滤，默认为空，表示不设置过滤条件

    - 参数 CMDLocation 仅支持按复制组 ID（对应参数"GroupID"）和复制组名（对应参数"GroupName"）进行过滤。
    - 如果参数 CMDLocation 显式设置，参数 Detail 将自动设置为 true。

    格式：`CMDLocation: {GroupName: "group1"}`

- SubCollections（ *string/array* ）：对访问计划的结果按照一个或多个子集合进行过滤，默认为空，表示不设置过滤条件

    - 该参数仅在涉及主、子集合的访问计划中生效。
    - 指定该参数时，需同时指定参数 Expand 为 true。

    格式：`SubCollections: ["subcs.subcl1", "subcs.subcl2"]`

- Search（ *boolean* ）：是否展示[访问计划的搜索过程][cost_estimation]，默认值为 false，表示不展示

    如果参数 Search 显式设置，参数 Detail 和 Expand 自动设置为 true。

    格式：`Search: true`

- Evaluate（ *boolean* ）：是否展示查询优化器的[推演公式][Evaluate]，默认值为 false，表示不展示

    如果参数 Evaluate 显式设置，参数 Detail 、Search 和 Expand 将自动设置成 true。

    格式：`Evaluate: true`

- Abbrev（ *boolean* ）：是否简略输出过长的字符串，默认值为 false，表示不简略输出

    格式：`Abbrev: true`

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取查询的访问计划，字段说明可参考[查看访问计划][explain]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

- 获取查询的普通访问计划

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

- 获取查询的详细访问计划

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

- 指定参数 Run 为 true，执行并获取查询的详细访问计划

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

- 指定参数 Expand 为 true，获取详细访问计划中的扩展信息

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

- 指定参数 Search 为 true，获取详细访问计划的搜索过程

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

- 指定参数 CMDLocation，获取查询在复制组 group1 上的详细访问计划

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
     本文使用的所有引用及链接
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

