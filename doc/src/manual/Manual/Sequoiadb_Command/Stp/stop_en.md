## NAME

stop - stop STP service

## SYNOPSIS

**stp.stop()**

## CATEGORY

Stp

## DESCRIPTION

This function is used to stop the STP service to which the current stp object is connected.

## PARAMETERS

None

## RETURN VALUE

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

## ERRORS

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

## VERSION

v5.0 and above

## EXAMPLES

Stop the local STP service.

```lang-javascript
> var stp = new Stp 
> stp.stop()
```

[^_^]:
    Links:
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
