##NAME##

addAHostMap - Add a hostname to ip address mapping to the host file

##SYNOPSIS##

***System.addAHostMap( \<hostname\>, \<ip\>, \[isReplace\] )***

##CATEGORY##

System

##DESCRIPTION##

Add a hostname to ip address mapping to the host file

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| hostname     | string   | ---     | hostname       | yes       |
| ip     | string   | ---          | ip address     | yes       |
| isReplace | boolean  | true        | whether to replace the target mapping relationship | not      |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Add a hostname to ip address mapping to the host file

```lang-javascript
> System.addAHostMap( "hostname", "1.1.1.1" )
```