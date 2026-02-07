##NAME##

primarySave - save primary node information of matched replica groups in the cluster

##SYNOPSIS##

**SdbDC.primarySave([option], [filename])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to retrieve primary node information of matched replica groups in the cluster and return it as an object. If a filename is specified, the result will also be saved to the file. The saved result can be used by [primaryRestore()][primaryRestore] to restore primary nodes.

##PARAMETERS##

option ( *object, optional* )

Specify filter conditions through the parameter "option". When set to null or not specified, primary node information of all replica groups will be retrieved:

- HostName ( *string* ): Filter by hostname

    Filter replica groups that contain nodes on the specified host.

    Format: `HostName: "sdbserver"`

- Domain ( *string | string array* ): Filter by domain

    Further filter replica groups by domain on top of the HostName filter. Only primary node information of replica groups belonging to the specified domain(s) will be retrieved.

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

filename ( *string, optional* )

Specify the file path to save the result. If specified, the result will be written to the file in JSON format.

> **Note:**
>
> - The coordinator group (SYSCoord) and spare group (SYSSpare) are excluded from the result.
> - If the specified Domain does not exist or has no replica groups, an exception will be thrown.
> - If a replica group has no primary node, it will be counted as failed.

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that successfully retrieved primary node information |
| IgnoredNum | number | Number of ignored replica groups |
| FailedNum | number | Number of failed replica groups (no primary node or node unreachable) |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |
| Detail | array | List of primary node details |

Each element in the Detail array contains the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| GroupName | string | Replica group name |
| PrimaryNode | string | Primary node address in the format hostname:svcname |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `primarySave()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error | Check whether the parameter type is correct |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Get primary node information of all replica groups

```lang-javascript
> var dc = db.getDC()
> dc.primarySave()
{
  "MatchedNum": 4,
  "SucceedNum": 4,
  "IgnoredNum": 0,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "SYSCatalogGroup",
      "PrimaryNode": "sdbserver:30020"
    },
    {
      "GroupName": "db1",
      "PrimaryNode": "sdbserver:20010"
    },
    {
      "GroupName": "db2",
      "PrimaryNode": "sdbserver:42000"
    },
    {
      "GroupName": "db3",
      "PrimaryNode": "sdbserver:21020"
    }
  ]
}
```

**Example 2:** Filter by hostname and save to file

```lang-javascript
> var dc = db.getDC()
> dc.primarySave({HostName: "sdbserver"}, "/tmp/primary.json")
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "PrimaryNode": "sdbserver:20010"
    },
    {
      "GroupName": "db2",
      "PrimaryNode": "sdbserver:42000"
    },
    {
      "GroupName": "db3",
      "PrimaryNode": "sdbserver:21020"
    }
  ]
}
```

**Example 3:** Use null as option and save to file

```lang-javascript
> var dc = db.getDC()
> dc.primarySave(null, "/tmp/primary.json")
```

**Example 4:** Filter by domain

```lang-javascript
> var dc = db.getDC()
> dc.primarySave({Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "PrimaryNode": "sdbserver:20010"
    },
    {
      "GroupName": "db2",
      "PrimaryNode": "sdbserver:42000"
    }
  ]
}
```

[^_^]:
    Links
[primaryRestore]:manual/Manual/Sequoiadb_Command/SdbDC/primaryRestore.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
