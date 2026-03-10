##NAME##

setActiveLocation - set the ActiveLocation of the cluster

##SYNOPSIS##

**SdbDC.setActiveLocation(\<location\>, [options])**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to set the specified location as ActiveLocation in the cluster.

##PARAMETERS##

location ( *string, required* )

Location name.

- The specified location needs to exist in the current cluster.
- When the value is an empty string, it means to delete the ActiveLocation of the current cluster.

options ( *object, optional* )

Specify filter conditions through the parameter "options":

- Domain ( *string | string array* ): Filter by domain, only operate on data groups belonging to the specified domain(s). A single domain name or an array of domain names can be specified

    Format: `Domain: "mydomain"` or `Domain: ["domain1", "domain2"]`

##RETURN VALUE##

When the function executes successfully, it returns a BSON object containing the following fields:

| Field Name | Type | Description |
| ---------- | ---- | ----------- |
| MatchedNum | number | Number of matched replica groups |
| SucceedNum | number | Number of replica groups that successfully set ActiveLocation |
| IgnoredNum | number | Number of ignored replica groups |
| FailedNum | number | Number of replica groups that failed to set ActiveLocation |
| FailedGroups | array | List of failed replica group names (returned when FailedNum > 0) |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setActiveLocation()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error. | Check whether the parameter type is correct. |
| -259 | SDB_OUT_OF_BOUND | Required parameters not specified. | Check whether missing required parameters. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

**Example 1:** Set location "GuangZhou" as the cluster's ActiveLocation

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("GuangZhou")
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 2:** Delete the cluster's ActiveLocation

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("")
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 3:** Set ActiveLocation only for data groups in the specified domain

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("GuangZhou", {Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**Example 4:** Set ActiveLocation with some replica groups failing

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("Beijing")
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group3", "group4"]
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md