##NAME##

startMaintenanceMode - start Maintenance mode in the cluster

##SYNOPSIS##

**SdbDC.startMaintenanceMode(\<options\>)**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to start the Maintenance mode for nodes in specified replica groups within the cluster. The nodes in Maintenance mode do not participate in replica group elections as well as the calculation of replSize.

> **Note:**
>
> Cannot set Maintenance mode for all nodes in a replica group.

##PARAMETERS##

options ( *object, required* )

Specify the Maintenance mode attributes through the parameter "options":

- HostName ( *string* ): The effective host for Maintenance mode (mutually exclusive with Location)

    The specified hostname needs to exist in the current replica group, and all nodes on this host will start Maintenance mode.

    Format: `HostName: "sdbserver"`

    > **Note:**  
    >
    > The primary node of catalog replica group (Catalog primary node) cannot start Maintenance mode unless using the Enforced parameter.

- Location ( *string* ): The effective location for Maintenance mode (mutually exclusive with HostName)

    The specified location needs to exist in the current cluster, and all nodes in replica groups within this location will start Maintenance mode.

    Format: `Location: "GuangZhou"`

- MinKeepTime ( *number, required* ): Minimum keep time for Maintenance mode. The value range is [1, 10080], and the unit is minutes.

    Format: `MinKeepTime: 100`

- MaxKeepTime ( *number, required* ): Maximum keep time for Maintenance mode. The value range is [1, 10080], and the unit is minutes.

    - MaxKeepTime must be greater than or equal to MinKeepTime
    - The system will automatically stop Maintenance mode when MaxKeepTime is reached

    Format: `MaxKeepTime: 200`

- Enforced ( *boolean* ): Whether to force start Maintenance mode, default is false.

    - When set to true, allows starting Maintenance mode for catalog replica group primary nodes
    - Special attention should be paid to system availability impact in forced mode

    Format: `Enforced: true`

- Domain ( *string | string array* ): Filter by domain, only operate on data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

> **Note:**
>
> When the replica group has a primary node, the primary node will not be changed.
> About time parameters:
> - Both MinKeepTime and MaxKeepTime are required parameters
> - MinKeepTime value should be less than or equal to MaxKeepTime
> - After successfully starting Maintenance mode, nodes will remain in Maintenance mode until MinKeepTime is reached
> - During the MinKeepTime-MaxKeepTime period, if node status is normal, Maintenance mode will be automatically released
> - After MaxKeepTime expires, nodes will forcibly exit Maintenance mode

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that successfully started Maintenance mode |
| IgnoredNum | number | Number of ignored replica groups |
| FailedNum | number | Number of replica groups that failed to start Maintenance mode |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `startMaintenanceMode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or parameter value out of range | Check whether the parameter type and value range are correct |
| -259 | SDB_OUT_OF_BOUND | Required parameters MinKeepTime or MaxKeepTime not specified | Check for missing required parameters |
| -334 | SDB_OPERATION_CONFLICT | Replica group is in Critical mode or primary node cannot start Maintenance mode | Check replica group status, or use Enforced parameter to force execution |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Start Maintenance mode for specified location

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 2:** Start Maintenance mode for specified host

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({HostName: "sdbserver", MinKeepTime: 60, MaxKeepTime: 300})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 3:** Force start Maintenance mode for catalog replica group primary node

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({HostName: "sdbserver", MinKeepTime: 30, MaxKeepTime: 120, Enforced: true})
{
  "MatchedNum": 1,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 4:** Start Maintenance mode only for data groups in the specified domain

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000, Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 5:** Start Maintenance mode with some replica groups failing

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group3", "group4"]
}
```

**Example 6:** View started Maintenance mode

```lang-javascript
> db.list(SDB_LIST_GROUPMODES )
{
  "GroupID": 1001,
  "GroupName": "group1",
  "GroupMode": "maintenance",
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
  "GroupMode": "maintenance",
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