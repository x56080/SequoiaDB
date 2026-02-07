##NAME##

reelect - reelect primary nodes for matched replica groups in the cluster

##SYNOPSIS##

**SdbDC.reelect(\<option\>)**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to batch reelect primary nodes for replica groups that match the specified conditions in the cluster. Target replica groups can be filtered by hostname or location, and optionally further filtered by domain.

##PARAMETERS##

option ( *object, required* )

Specify the reelect conditions through the parameter "option":

- HostName ( *string* ): Filter by hostname (mutually exclusive with Location)

    Filter replica groups that contain nodes on the specified host, and reelect primary nodes in those replica groups.

    Format: `HostName: "sdbserver"`

- Location ( *string* ): Filter by location (mutually exclusive with HostName)

    Filter replica groups that contain nodes in the specified location, and reelect primary nodes in those replica groups.

    Format: `Location: "GuangZhou"`

- Domain ( *string | string array* ): Filter by domain

    Further filter replica groups by domain on top of the HostName or Location filter. Only replica groups belonging to the specified domain(s) will be reelected.

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

- Seconds ( *number* ): Reelect timeout, default is 30, in seconds

    Format: `Seconds: 60`

- Level ( *number* ): Reelect wait level, value range: [1, 3], default is 3

    - 1: Wait for the current write operation to finish and block subsequent write operations
    - 2: Wait for write cursors (LOB operations) to finish
    - 3: Wait for transactions to finish

    Format: `Level: 3`

- Mode ( *number* ): Node specification mode, default is 1

    - 0: Exclude mode, exclude the specified nodes from being elected as primary
    - 1: Specify mode, specify the nodes to be elected as primary

    Format: `Mode: 1`

> **Note:**
>
> - Either HostName or Location must be specified, but not both.
> - The coordinator group (SYSCoord) and spare group (SYSSpare) are excluded from the reelect operation.
> - If the specified HostName or Location does not exist in the cluster, an exception will be thrown.
> - If the specified Domain does not exist or has no replica groups, an exception will be thrown.
> - The Seconds, Level, and Mode parameters are passed to the [rg.reelect()][rg_reelect] operation of each replica group.

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that successfully reelected |
| IgnoredNum | number | Number of ignored replica groups (primary node unchanged) |
| FailedNum | number | Number of replica groups that failed to reelect |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `reelect()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or missing required parameter | Check whether the parameter type is correct and ensure HostName or Location is specified |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Reelect primary nodes for replica groups on a specified host

```lang-javascript
> var dc = db.getDC()
> dc.reelect({HostName: "sdbserver"})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 2:** Reelect primary nodes for replica groups in a specified location

```lang-javascript
> var dc = db.getDC()
> dc.reelect({Location: "GuangZhou"})
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 2,
  "FailedNum": 0
}
```

**Example 3:** Reelect with domain filter

```lang-javascript
> var dc = db.getDC()
> dc.reelect({HostName: "sdbserver", Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 1,
  "FailedGroups": ["group2"]
}
```

**Example 4:** Reelect with multiple domain filter

```lang-javascript
> var dc = db.getDC()
> dc.reelect({Location: "GuangZhou", Domain: ["domain1", "domain2"]})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
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
[rg_reelect]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md
