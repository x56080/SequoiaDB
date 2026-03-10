##NAME##

startCriticalMode - start Critical mode in the cluster

##SYNOPSIS##

**SdbDC.startCriticalMode(\<options\>)**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used for cluster disaster recovery, to start Critical mode for replica groups or location sets that do not meet the election conditions, and elect the primary node within the specified node range.

##PARAMETERS##

options ( *object, required* )

Specify the Critical mode attributes through the parameter "options":

- HostName ( *string* ): The effective host for Critical mode (mutually exclusive with Location).

    The specified hostname needs to exist in the current cluster, and one node from each replica group on this host will start Critical mode.

    Format: `HostName: "sdbserver"`

- Location ( *string* ): The effective location for Critical mode (mutually exclusive with HostName).

    The specified location needs to exist in the current cluster, and all nodes in replica groups within this location will start Critical mode.

    Format: `Location: "GuangZhou"`

    > **Note:**  
    >
    > When the catalog replication group starts Critical mode, the effective node must contain the primary node of the current replication group.

- MinKeepTime ( *number, required* ): Minimum keep time for Critical mode. The value range is [1, 10080], and the unit is minutes.

    Format: `MinKeepTime: 100`

- MaxKeepTime ( *number, required* ): Maximum keep time for Critical mode. The value range is [1, 10080], and the unit is minutes.

    - MaxKeepTime must be greater than or equal to MinKeepTime
    - The system will automatically stop Critical mode when MaxKeepTime is reached

    Format: `MaxKeepTime: 200`

- Enforced ( *boolean* ): Whether to force start Critical mode, default is false.

    This parameter is optional, and the values are as follows:

    - true: The replication group will forcibly generate a primary node within the effective node range of the Critical mode. If there is a node with a higher LSN outside the effective node range, the forced execution will cause the data to be rolled back.
    - false: The primary node is not forced to be within the effective node range of Critical mode, and the active node with the current higher LSN will still be elected as the primary node.

    Format: `Enforced: true`

- Domain ( *string | string array* ): Filter by domain, only operate on data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

> **Note:**
>
> When the replica group has a primary node, the primary node will not be changed.
> About time parameters:
> - Both MinKeepTime and MaxKeepTime are required parameters
> - MinKeepTime value should be less than or equal to MaxKeepTime
> - After successfully starting Critical mode, the replica group will remain in Critical mode until MinKeepTime is reached
> - During the MinKeepTime-MaxKeepTime period, if most nodes in the replica group are normal, Critical mode will be automatically released
> - After MaxKeepTime expires, the replica group will forcibly exit Critical mode

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that successfully started Critical mode |
| IgnoredNum | number | Number of ignored replica groups |
| FailedNum | number | Number of replica groups that failed to start Critical mode |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `startCriticalMode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or parameter value out of range | Check whether the parameter type and value range are correct |
| -259 | SDB_OUT_OF_BOUND | Required parameters MinKeepTime or MaxKeepTime not specified | Check for missing required parameters |
| -334 | SDB_OPERATION_CONFLICT | Replica group is in Maintenance mode or parameter range error | Check replica group status, or check whether the primary node of the catalog is within the effective node range of the Critical mode |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Start Critical mode for specified location

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 2:** Start Critical mode for specified host

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({HostName: "sdbserver", MinKeepTime: 60, MaxKeepTime: 300})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 3:** Force start Critical mode

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({HostName: "sdbserver", MinKeepTime: 30, MaxKeepTime: 120, Enforced: true})
{
  "MatchedNum": 1,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 4:** Start Critical mode only for data groups in the specified domain

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000, Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 5:** Start Critical mode with some replica groups failing

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group3", "group4"]
}
```

**Example 6:** View started Critical mode

```lang-javascript
> db.list(SDB_LIST_GROUPMODES )
{
  "GroupID": 1001,
  "GroupName": "group1",
  "GroupMode": "critical",
  "Properties": [
    {
      "NodeID": 1002,
      "NodeName": "sdbserver:11820",
      "MinKeepTime": "2024-02-03-14.30.00",
      "MaxKeepTime": "2024-02-03-19.30.00",
      "UpdateTime": "2024-02-03-14.30.00"
    }
  ]
}
{
  "GroupID": 1002,
  "GroupName": "group2",
  "GroupMode": "critical",
  "Properties": [
    {
      "NodeID": 1003,
      "NodeName": "sdbserver:11830",
      "MinKeepTime": "2024-02-03-14.30.00",
      "MaxKeepTime": "2024-02-03-19.30.00",
      "UpdateTime": "2024-02-03-14.30.00"
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