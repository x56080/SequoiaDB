##NAME##

stopMaintenanceMode - stop data center maintenance mode

##SYNOPSIS##

**dc.stopMaintenanceMode([options])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used for disaster recovery. It stops the Maintenance mode of the cluster and restores normal operating state.

##PARAMETERS##

options ( *object, optional* )

Through the options parameter, you can specify the parameters for Maintenance mode:

- HostName ( *string* ): The hostname where Maintenance mode stops taking effect (mutually exclusive with Location)

    The specified hostname must exist in the current cluster. All nodes on this host will stop Maintenance mode.

    Format: `HostName: "sdbserver"`

- Location ( *string* ): The location set where Maintenance mode stops taking effect (mutually exclusive with HostName)

    The specified location set must exist in the current cluster. All replication group nodes in this location set will stop Maintenance mode.

    Format: `Location: "GuangZhou"`

- Domain ( *string | string array* ): Filter by domain, only operate on data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ------ | ---- | ---- |
| MatchedNum | number | Number of matched replication groups |
| SucceedNum | number | Number of replication groups that successfully stopped Maintenance mode |
| IgnoredNum | number | Number of ignored replication groups |
| FailedNum | number | Number of replication groups that failed to stop |
| FailedGroups | array | List of failed replication group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `stopMaintenanceMode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or parameter value out of range | Check whether the parameter type and value range are correct |
| -334 | SDB_OPERATION_CONFLICT | Replication group is in Critical mode | Check replication group status |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Stop cluster Maintenance mode

```lang-javascript
> var dc = db.getDC()
> dc.stopMaintenanceMode()
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 2:** Stop Maintenance mode only for data groups in the specified domain

```lang-javascript
> var dc = db.getDC()
> dc.stopMaintenanceMode({Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
