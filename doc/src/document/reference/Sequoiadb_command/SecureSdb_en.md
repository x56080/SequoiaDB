##NAME##

SecureSdb - Create a SecureSdb object.

##SYNOPSIS##

***new SecureSdb( [hostname], [svcname] )***

***var securesdb = new SecureSdb( [hostname], [svcname], [username], [password] )***

##CATEGORY##

SecureSdb

##DESCRIPTION##

Create a SecureSdb object.

>**Note:**

>1. SecureSdb is subclass of Sdb and SecureSdb object uses SSL connection.

>2. The method and syntax of the SecureSdb object and the Sdb object are the same.

##PARAMETERS##

| Name     | Type   | Default            | Description  | Required or not |
| -------- | ------ | ------------------ | ------------ | --------------- |
| hostname | string | localhost          | IP address   | not             |
| svcname  | int    | local sdbcm's port | sdbcm's port | not             |
| username | string | empty   | username of sequoiadb   | not             |
| password | string | empty   | password of sequoiadb   | not             |

##RETURN VALUE##

On success, return a SecureSdb.

On error, exception will be thrown.

##ERRORS##

When exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

1. Create a SecureSdb object.

	```lang-javascript
 	> var securesdb = new SecureSdb( "192.168.20.71", 11790 )
 	```

2. Create a SecureSdb object with a username and password.

	```lang-javascript
 	> var securesdb = new SecureSdb("sdbserver1",11810,"sdbadmin","123")
 	```
