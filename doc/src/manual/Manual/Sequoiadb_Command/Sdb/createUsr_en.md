##NAME##

createUsr - create database user

##SYNOPSIS##

**db.createUsr(\<name\>, \<password\>, [options])**

**db.createUsr(\<User\>, [options])**

**db.createUsr(\<CipherUser\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create database users to prevent illegal users from operating the database.

##PARAMETERS##

| Name       | Type     | Description       | Required or not |
| ---------- | -------- | ------------------| --------------- |
| name       | string   |  username         | required        |
| password   | string   |  password         | required        |
| User       | object   | [User][user] object   | required    |
| CipherUser | object   | [CipherUser][CipherUser] object | required |
| options    | object   |  extended options  | not            |

###options value###

| Attributes | Type   | Description                       |
| ---------- | ------ | --------------------------------- |
| AuditMask  | string | The configuration mask of the user [auditlog][auditlog], the default value is "SYSTEM\|DDL\|DCL", and the values are as follows:<br>ACCESS, CLUSTER, SYSTEM, DCL, DDL, DML, DQL, INSERT, UPDATE, DELETE, OTHER, ALL, NONE<br>● Supports using 'bitwise or'(\|) to connect multiple masks, and 'logic not'(\!) prohibits a mask.<br>● A value of "ALL" indicates that all configuration masks are selected.<br>● A value of "NONE" indicates that all configuration masks are prohibited. That is, the audit function is turned off. |
| Role       | String | User role. Currently only supports built-in roles in the system, the default value is "admin", and the value list: "admin", "monitor". "admin" is the administrator role, which can perform any operation. "monitor" is the monitoring role, which can only perform snapshot and list operations. |

> **Note:**
>
> - This interface can only be used in cluster mode.
> - When a user is created in the database, the username and password must be specified to connect to the database.
> - For database username and password restrications, refer to [database limit][database_limit].
> - The first user created in the database must be in the "admin" role.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, please refer to [Troubleshooting][faq].

##EXAMPLES##

* Create a user with username "sdbadmin" and password "sdbadmin", and set the auditlog mask.

    ```lang-javascript
    > db.createUsr("sdbadmin", "sdbadmin", {AuditMask: "DDL|DML|!DQL"})
    ```

* Use the User object to create a user with username "sdbadmin" and password "sdbadmin".

    ```lang-javascript
    > var a = User("sdbadmin", "sdbadmin")
    > db.createUsr(a)
    ```

* Use the CipherUser object to create a user with username "sdbadmin" and password "sdbadmin"(The user information with username "sdbadmin" and password "sdbadmin" in the ciphertext file. For details on how to add and delete ciphertext information in the ciphertext file, refer to [sdbpasswd][passwd]).

    ```lang-javascript
    > var a = CipherUser("sdbadmin")
    > db.createUsr(a)
    ```

[^_^]:
     Links
[user]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[cipherUser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[passwd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md
[database_limit]:manual/Manual/sequoiadb_limitation.md#数据库
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md