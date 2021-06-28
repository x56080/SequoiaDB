##NAME##

getSessionAttr - get the session attributes

##SYNOPSIS##

**db.getSessionAttr()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the session attributes.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type  BSONObj. Users can get a list of session attributes details through this object. For field descriptions, refer to setSessionAttr().

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

Get the session attributes.

```lang-javascript
> db.getSessionAttr()
{
  "PreferedInstance": "M",
  "PreferedInstanceMode": "random",
  "PreferedStrict": false,
  "Timeout": -1,
  "TransIsolation": 0,
  "TransTimeout": 60,
  "TransUseRBS": true,
  "TransLockWait": false,
  "TransAutoCommit": false,
  "TransAutoRollback": true,
  "TransRCCount": true,
  "Source": ""
}
```

[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md