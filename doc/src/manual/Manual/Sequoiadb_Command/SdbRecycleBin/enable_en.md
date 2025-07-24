##NAME##

enable - enable the recycle bin

##SYNOPSIS##

**db.getRecycleBin().enable()**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to enable the recycle bin.

##PARAMETERS##

None.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

<<<<<<< HEAD
v3.6 and above
=======
v5.0.3 and above
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

##EXAMPLES##

Enable the recycle bin.

```lang-javascript
> db.getRecycleBin().enable()
```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
