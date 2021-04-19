## NAME

reelect - reelect the master in the server 

## SYNOPSIS

**stp.reelect([options])**

## CATEGORY

Stp

## DESCRIPTION

This function is used to reelect the master node in the server group where the current STP node is located. The number of surviving nodes in the server group must exceed 1/2 of the total number of nodes in other to be elected.

## PARAMETERS

options ( *object, optional* )

Other optional parameters can be set through the option parameter:

- Seconds (number): Specify the election timeout period, the election will be completed within the specified time, the unit is seconds, the default value is 30.

    The value of this parameter must be greater than or equal to 10. Otherwise an error will be reported when the relevant statement is executed.

    Format: `Seconds: 60`

- HostName (string): Specify the host name of the desired master node.

    Format: `HostName: "sdbserver"`

## RETURN VALUE

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

## ERRORS

The common exceptions of 'reelect()' function are as follows:

| Error Code | Error Type  | Description | Solution |
| ---------- | ----------  | ----------- | -------- |
| -13        | SDB_TIMEOUT | The election was not completed within the specified time. | - |
 
When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

## VERSION

v5.0 and above

## EXAMPLES

Reelect in the server group where the current STP node is located, and specify the election timeout period as 60s. 

```lang-javascript
> var stp = new Stp()
> stp.reelect({Seconds: 60})
```

[^_^]:
    Links
[getServers]:manual/Manual/Sequoiadb_Command/Stp/getServers.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
