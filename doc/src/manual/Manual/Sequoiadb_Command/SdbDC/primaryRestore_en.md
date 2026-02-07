##NAME##

primaryRestore - restore primary nodes of replica groups based on saved information

##SYNOPSIS##

**SdbDC.primaryRestore(\<planobjOrFile\>, [option])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to restore primary nodes of replica groups based on the information saved by [primarySave()][primarySave]. It accepts the BSON object returned by primarySave, a plain JSON object, or a file path containing the saved result.

##PARAMETERS##

planobjOrFile ( *object | BSONObj | string, required* )

Specify the primary node restore plan. The following types are supported:

- object / BSONObj: The result object returned by [primarySave()][primarySave], which must contain a Detail array
- string: The file path where [primarySave()][primarySave] saved the result. The function will read and parse the file content as a JSON object

option ( *object, optional* )

Specify filtering conditions for the data groups to restore, as well as additional reelect parameters through the parameter "option":

- HostName ( *string* ): Filter by hostname, only restore data groups whose PrimaryNode hostname matches

    Format: `HostName: "sdbserver"`

- Domain ( *string | array* ): Filter by domain, only restore data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

- Seconds ( *number* ): Reelect timeout, default is 30, in seconds

    Format: `Seconds: 60`

- Level ( *number* ): Reelect wait level, value range: [1, 3], default is 3

    - 1: Wait for the current write operation to finish and block subsequent write operations
    - 2: Wait for write cursors (LOB operations) to finish
    - 3: Wait for transactions to finish

    Format: `Level: 3`

> **Note:**
>
> - The planobjOrFile must contain a valid Detail array, and each element must include GroupName and PrimaryNode fields.
> - The PrimaryNode format is hostname:svcname. The function will parse the HostName and ServiceName for the reelect operation.
> - If the GroupName or PrimaryNode of an entry in Detail is missing, that entry will be skipped.
> - HostName filtering matches against the hostname in PrimaryNode; Domain filtering matches against the domain the data group belongs to.

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of replica groups matched after filtering |
| SucceedNum | number | Number of replica groups that successfully restored primary nodes |
| IgnoredNum | number | Number of ignored replica groups (primary node unchanged) |
| FailedNum | number | Number of replica groups that failed to restore |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `primaryRestore()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or missing Detail field | Check whether the parameter type is correct and ensure the object contains a valid Detail array |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8.6 and above

##EXAMPLES##

**Example 1:** Restore primary nodes using the object returned by primarySave

```lang-javascript
> var dc = db.getDC()
> var result = dc.primarySave({HostName: "sdbserver"})
> dc.primaryRestore(result)
{
  "MatchedNum": 3,
  "SucceedNum": 2,
  "IgnoredNum": 1,
  "FailedNum": 0
}
```

**Example 2:** Restore primary nodes from a file

```lang-javascript
> var dc = db.getDC()
> dc.primarySave(null, "/tmp/primary.json")
> dc.primaryRestore("/tmp/primary.json")
{
  "MatchedNum": 4,
  "SucceedNum": 4,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 3:** Restore with reelect options

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json", {Seconds: 60, Level: 1})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 4:** Filter by hostname, only restore data groups on the specified host

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json", {HostName: "sdbserver1"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 5:** Filter by domain, only restore data groups in the specified domain

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json", {Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 6:** Restore with some replica groups failing

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json")
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["db3", "db4"]
}
```

[^_^]:
    Links
[primarySave]:manual/Manual/Sequoiadb_Command/SdbDC/primarySave.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
