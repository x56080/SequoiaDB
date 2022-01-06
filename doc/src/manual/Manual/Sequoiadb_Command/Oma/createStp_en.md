##NAME##

createStp - create STP service

##SYNOPSIS##

**oma.createStp( [config] )**

##CATEGORY##

Oma

##DESCRIPTION##

This function is used to create STP service(Sequence Time Protocol service) in target host of sdbcm.

##PARAMETERS##

config ( *object, Optional* )

STP configuration information.

 - port (number): STP listening port. The default value is 9622.

 - serverlist (string): STP server list. The default value is null, which means the host name of this machine.

 - role (string): STP role. The default value is "server".

 - syncinterval (number): STP synchronize interval in seconds. The default value is 60.

 - maxtimeerror (number): STP max time error in microseconds, range from 1000 to 10000000. The default value is 50000.

 - diaglevel (number): STP dialog level. The default value is 3.

##RETURN VALUE##

When the function executes successfully, it will return an object of STP.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createStp()` function are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error. | Check if the parameters are correct. |
| -145 | SDBCM_NODE_EXISTED | STP already exist. | Check config path of STP `<INSTALL_DIR>/conf/stp/stp.conf` if the STP already exists. |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Create and start a STP process.

```lang-javascript
> var oma = new Oma("localhost",11790)
> oma.createStp()
> oma.startStp()
```

[^_^]:
    Links:
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[stp_config]:manual/Distributed_Engine/Architecture/Stp/Tools/stp.md#参数说明
