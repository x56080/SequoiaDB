
##NAME##

delUser - Delete a system user

##SYNOPSIS##

***System.delUser( \<users\> )***

##CATEGORY##

System

##DESCRIPTION##

Delete a system user

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| users | JSON   | ---    |  user information  | yes   |

The detail description of 'users' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | ---------------- |
| name    | string |   yes  | { "name": newUser }     | user name  |
| group    | string |  not   | { "group": groupname }     | user group name  |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Delete a system user

```lang-javascript
> System.addUser( { "name": "newUser" } )
> System.delUser( { "name": "newUser" } )
```