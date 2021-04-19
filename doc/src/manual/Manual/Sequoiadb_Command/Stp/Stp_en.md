## NAME

Stp - STP service process object

## SYNOPSIS

**var stp = new Stp([hostname],[svcname])**

## CATEGORY

Stp

## DESCRIPTION

This function is used to create a new STP service process object to connect to STP nodes.

## PARAMETERS

* hostname ( *string, optional* )

   The hostname of the host where the target STP is located.

* svcname ( *number/string, optional* )

   The port used by the target STP, the default port is 9622.

## RETURN VALUE

When the function executes successfully, it will return an object of Stp.

When the function fails, an exception will be thrown and an error message will be printed.

## ERRORS

The common exceptions of `Stp()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | Network error. | Check the hostname and the port for STP is reachable. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

## VERSION

v5.0 and above

## EXAMPLES

- Connect to the local STP service process object.

    ```lang-javascript
    > var stp = new Stp()
    ```

- STP service process object connected to the specified machine. 

    ```lang-javascript
    > var stp = new Stp("sdbserver", 9622)
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
