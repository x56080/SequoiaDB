##NAME##

delUser - delete an operating system user

##SYNOPSIS##

**System.delUser(\<users\>)**

##CATEGORY##

System

##DESCRIPTION##

This function is used to delete an operating system users.

##PARAMETERS##

users ( *object, required* )

Parameter users can be used to set the user to be deleted:

- name ( *string* ): User name. This parameter is required.

    Format: `name: "username"`

- isRemoveDir ( *boolean* ): Whether to remove the user directory, the defual is false.

    Format: `isRemoveDir: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

Delete the specified system user.

```lang-javascript
> System.delUser({name: "newUser"})
```