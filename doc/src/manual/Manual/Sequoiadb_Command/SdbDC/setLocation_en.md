##NAME##

setLocation - modify the location information of nodes in the cluster

##SYNOPSIS##

**SdbDC.setLocation(\<hostName\>, \<location\>, [options])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to modify the location information of nodes on the specified host in the cluster.

##PARAMETERS##

hostName ( *string, required* )

The hostname to modify location information for. The specified hostname needs to exist in the current cluster, and the location information of all nodes on this host will be modified.

location ( *string, required* )

New location information.

- The maximum length of location information is limited to 256 bytes
- When the value is an empty string, it means to delete the location information of nodes on the specified host

options ( *object, optional* )

Specify filter conditions through the parameter "options":

- Domain ( *string | string array* ): Filter by domain, only operate on data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that successfully modified location information |
| IgnoredNum | number | Number of ignored replica groups |
| FailedNum | number | Number of replica groups that failed to modify |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setLocation()` function are as follows:

| Error Code | Error Type | Description | Solution |
| -------- | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error or location name length exceeds 256 bytes | Check whether the parameter type and location name length are correct |
| -259 | SDB_OUT_OF_BOUND | Required parameters HostName or Location not specified | Check whether required parameters are missing |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

**Example 1:** Modify the location information of nodes on the specified host to "GuangZhou"

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "GuangZhou")
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 2:** Delete the location information of nodes on the specified host

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "")
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 3:** Modify location information only for data groups in the specified domain

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "GuangZhou", {Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 4:** Modify location information with some replica groups failing

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "Beijing")
{
  "MatchedNum": 3,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group1", "group2"]
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md