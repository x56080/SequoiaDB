##NAME##

createUsr - Create a database user to prevent illegal users from illegally operating the database.

##SYNOPSIS##

***db.createUsr( \<name\>, \<password\>, [options] )***

***db.createUsr( \<User\>, [options] )***

***db.createUsr( \<CipherUser\>, [options] )***

##CATEGORY##

Sdb

##DESCRIPTION##

Create a database user to prevent illegal users from illegally operating the database.

##PARAMETERS##

| Name       | Type     | Default | Description       | Required or not |
| ---------- | -------- | ------- | ----------------- | --------------- |
| name       | string   | ---     | username          | yes             |
| password   | string   | ---     | password          | yes             |
| User       | object   | ---     | [User](reference/Sequoiadb_command/AuxiliaryObjects/User.md) object       | yse             |
| CipherUser | object   | ---     | [CipherUser](reference/Sequoiadb_command/AuxiliaryObjects/CipherUser.md) object | yes             |
| options    | Json     | null    | extended options  | not             |

The detail description of 'options' parameter is as follow:

| Attributes | Type   | Description                       | Required or not |
| ---------- | ------ | --------------------------------- | --------------- |
| AuditMask  | string | user audit log configuration mask | not             |

>Note：

>* This interface can only be used in cluster mode.

>* If the database has created a user, you must specify a username and password to connect to the database.

>* AuditMask's value are as follow: ACCESS、CLUSTER、SYSTEM、DML、DDL、DCL、DQL、INSERT、DELETE、UPDATE、OTHER. You can combine multiple values with '\|'. 'ALL' means that all mask items are turned on, and 'NONE' means that no mask items are turned on. If an item in the user audit log is not configured, the configuration of the corresponding mask item on the node is inherited. You can also use '!' to disable inheritance of this mask( e.g: "!DDL|DML" ).

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

1. Create a user with username 'sdbadmin' and password 'sdbadmin', and set the audit log mask.

	```lang-javascript
 	> db.createUsr( "sdbadmin", "sdbadmin", { AuditMask: "DDL|DML|!DQL" } )
 	```
 
2. Create a user with username 'sdbadmin' and password 'sdbadmin' using User object.

	```lang-javascript
 	> var a = User( "sdbadmin", "sdbadmin" )
    > db.createUsr( a )
 	```

3. Create a user with username 'sdbadmin' and password 'sdbadmin' using CipherUser object ( User information with username 'sdbadmin' and password 'sdbadmin' must exist in the cipher test file. For details on how to add and delete cipher test information in cipher test file, please see [sdbpasswd](database_management/tools/sdbpasswd.md) for details ).

	```lang-javascript
    > var a = CipherUser( "sdbadmin" )
    > db.createUsr( a )
 	```
