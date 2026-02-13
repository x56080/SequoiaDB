##NAME##

reelectAnalyze - analyze the primary node distribution of replica groups in the cluster, or switch primary nodes to optimal nodes

##SYNOPSIS##

**SdbDC.reelectAnalyze([option], [run])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to analyze whether the primary node distribution of replica groups in the cluster is reasonable. By querying the reelect weight (RunStatusWeight) of each node, it determines whether the current primary node is on the node with the highest weight. If not, a reelect plan will be generated. When run is set to true, the reelect operation will also be executed.

##PARAMETERS##

option ( *object, optional* )

Specify filter conditions and analysis options through the parameter "option". When set to null or not specified, all data groups will be analyzed:

- HostName ( *string* ): Filter by hostname, only analyze data groups that contain nodes on the specified host

    Format: `HostName: "sdbserver"`

- Domain ( *string | string array* ): Filter by domain, only analyze data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

- FilterLevel ( *string* ): Filter level for selecting candidate nodes for weight comparison. Default is "Weight"

    - "GroupMode": Only consider "Critical" or "Maintenance" nodes
    - "Location": In addition to GroupMode, also include nodes with "ActiveLocation" and "AffinitiveLocation"
    - "Weight": Consider all nodes

    Format: `FilterLevel: "Location"`

- Rebalance ( *boolean* ): Whether to enable primary node balancing, default is true

    When enabled, primary nodes are balanced among multiple nodes with the same highest weight.

    Format: `Rebalance: true`

- Seconds ( *number* ): Reelect timeout, default is 30, in seconds. Only effective when run is true

    Format: `Seconds: 60`

- Level ( *number* ): Reelect wait level, value range: [1, 3], default is 3. Only effective when run is true

    - 1: Wait for the current write operation to finish and block subsequent write operations
    - 2: Wait for write cursors (LOB operations) to finish
    - 3: Wait for transactions to finish

    Format: `Level: 3`

run ( *boolean, optional* )

Specify whether to execute the reelect operation, default is false:

- false: Only analyze and output the reelect plan without actual execution
- true: Analyze and then execute the reelect operation

> **Note:**
>
> - When a group has no primary node, it will be counted as failed (FailedNum).
> - Rebalance only balances among candidate nodes with the same highest weight.
> - The Seconds and Level parameters are passed to the [rg.reelect()][rg_reelect] operation of each replica group.

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that need reelection (or have successfully reelected) |
| IgnoredNum | number | Number of ignored replica groups (current primary is already optimal or no candidate nodes) |
| FailedNum | number | Number of failed replica groups (no primary node or reelect failed) |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |
| Detail | array | List of reelect plan details |

Each element in the Detail array contains the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| GroupName | string | Replica group name |
| OldPrimary | string | Current primary node address in the format hostname:svcname |
| NewPrimary | string | Suggested (or switched) new primary node address in the format hostname:svcname |
| CausedBy | string | Reason for the reelection, possible values: "Critical", "ActiveLocation", "AffinitiveLocation", "Maintenance", "Weight", "Rebalance" |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `reelectAnalyze()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or invalid FilterLevel value | Check whether the parameter type is correct and ensure FilterLevel is one of GroupMode, Location, or Weight |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Analyze primary node distribution of all data groups

```lang-javascript
> var dc = db.getDC()
> dc.reelectAnalyze()
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 2,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "OldPrimary": "sdbserver1:20000",
      "NewPrimary": "sdbserver2:20000",
      "CausedBy": "Critical"
    },
    {
      "GroupName": "db3",
      "OldPrimary": "sdbserver1:20020",
      "NewPrimary": "sdbserver2:20020",
      "CausedBy": "Weight"
    }
  ]
}
```

**Example 2:** Analyze with GroupMode filter level

```lang-javascript
> var dc = db.getDC()
> dc.reelectAnalyze({FilterLevel: "GroupMode"})
{
  "MatchedNum": 4,
  "SucceedNum": 1,
  "IgnoredNum": 3,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "OldPrimary": "sdbserver1:20000",
      "NewPrimary": "sdbserver2:20000",
      "CausedBy": "Critical"
    }
  ]
}
```

**Example 3:** Filter by hostname and domain, then execute reelection

```lang-javascript
> var dc = db.getDC()
> dc.reelectAnalyze({HostName: "sdbserver1", Domain: "mydomain"}, true)
{
  "MatchedNum": 2,
  "SucceedNum": 1,
  "IgnoredNum": 1,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "OldPrimary": "sdbserver1:20000",
      "NewPrimary": "sdbserver2:20000",
      "CausedBy": "ActiveLocation"
    }
  ]
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[rg_reelect]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md
