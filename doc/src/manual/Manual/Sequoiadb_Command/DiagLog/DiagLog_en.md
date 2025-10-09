##NAME##

diaglog - Create a new DiagLog object

##SYNOPSIS##

**var diaglog = new DiagLog( [hostname], [svcname] )**

**var diaglog = new DiagLog( [hostname], [svcname], [username], [password] )**

**var diaglog = new DiagLog( [hostname], [svcname], [CipherUser] )**

##CATEGORY##

DiagLog

##DESCRIPTION##

Create a new DiagLog object.

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| ---------- | -------- | ------------------ | --------------- | -------- |
| hostname   | string   | localhost          | hostname          | not       |
| svcname    | int      | 11810              | service name       | not       |
| username   | string   |                  | user name          | not       |
| password   | string   |                  | password            | not       |
| CipherUser | object   | ---                | [CipherUser][cipher] object | not       |

##RETURN VALUE##

NULL

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

1. Connect to the SequoiaDB cluster on the default host. The hostname defaults to "localhost" and the svcname defaults to 11810.

	```lang-javascript
 	> var diaglog = new DiagLog()
 	```

2. Connect to the SequoiaDB cluster on the specified machine, the target machine is "sdbserver1".

	```lang-javascript
 	> var diaglog = new DiagLog( "sdbserver1", 11810 )
	```

3. Connect to the SequoiaDB cluster on the specified machine using the username and password.

	```lang-javascript
 	> var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
	```

4. Use the CipherUser object to connect to SequoiaDB on the specified machine.

   	```lang-javascript
    > var a = CipherUser( "sdbadmin" ).cipherFile( "/home/sdbadmin/passwd" )
 	>var diaglog = new DiagLog( "sdbserver1", 11810, a )
    ```