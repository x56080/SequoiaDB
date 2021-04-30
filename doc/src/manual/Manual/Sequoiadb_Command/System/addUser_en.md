
##NAME##

addUser - add an operating system user

##SYNOPSIS##

**System.addUser(\<users\>)**

##CATEGORY##

System

##DESCRIPTION##

This function is used to add an operating system user.

##PARAMETERS##

users ( *object, required* )

The user's attributes can be set through the users parameter:

- name ( *string* ): User name. This parameter is required.

    Format: `name: "username"`

- group ( *string* ): Specify user group.

    This parameter must be specified as an existing user group. If not specified, a user group with the same name as the parameter name will be created by default.

    Format: `group: "groupName"`

- additionGroup ( *string* ): Specify additional groups.

    This parameter must be specified as an existing user group.

    Format: `additionGroup: "groupName"`

- createDir ( *boolean* ): Whether to create a user directory, the default is false.

    Format: `createDir: true`

- dir ( *string* ): Specify the user directory, which takes effect only when the parameter createDir is true.

    This parameter cannot specify an existing directory. If not specified, a directory with the same name as the parameter name will be created in the `/home` directory as the user directory.

    Format: `dir: "userHomeDir"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2 and above

##EXAMPLES##

Add a new system user named "newUser". Specify the user group as root, and create a user directory `/home/newUser`.

```lang-javascript
> System.addUser({name: "newUser", group: "root", createDir: true, dir: "/home/newUser"})
```



[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md