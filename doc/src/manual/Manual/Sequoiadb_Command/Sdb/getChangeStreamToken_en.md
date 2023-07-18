##NAME##

getChangeStreamToken - Get a token for the latest position information of the change stream for the current node

##SYNOPSIS##

**Sdb.getChangeStreamToken()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get a token for the latest position information of the change stream for the current node.

##PARAMETERS##

None

##RETURN VALUE##

If the function executes successfully, it will return a StreamToken type object.

If the function execution fails, it will throw an exception and output error information.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v7.2.2 and above

##EXAMPLES##

```lang-javascript
// Get the latest position information of the change stream for the current node
var token = db.getChangeStreamToken();
```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[config]:manual/Manual/Database_Configuration/configuration_parameters.md