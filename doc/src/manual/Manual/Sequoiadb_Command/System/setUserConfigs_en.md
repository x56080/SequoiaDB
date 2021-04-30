
##NAME##

setUserConfigs - set the configuration of operating system user

##SYNOPSIS##

**System.setUserConfigs(\<options\>)**

##CATEGORY##

System

##DESCRIPTION##

This function is used to modify the user group, additional group, user directory and other configurations of the operating system user.

##PARAMETERS##

options ( *object, required* )

The user's attributes can be modified through the options parameter:

- name ( *string* ): Specify the user who needs to be modified. This parameter is required.

    Format: `name: "username"`

- group ( *string* ): Specify a new user group.

    This parameter must be specified as an existing user group.

    Format: `group: "groupName"`

- additionGroup ( *string* ): Specify a new additional group.

    This parameter must be specified as an existing user group.

    Format: `additionGroup: "groupName"`

- isAppend ( *boolean* ): Specify whether to append additional groups, the default is false.

    This parameter needs to be used with the additionGroup parameter. When additionGroup is specified and isAppend is set to true, the additional group of the user will be added. When additionGroup is specified and isAppend is set to false, the original group will be replaced.

    Format: `isAppend: true`

- isMove ( *boolean* ): Whether to move the newly specified directory, the default is false.

    When this parameter is true, the value of parameter dir must be specified.

    Format: `isMove: true`

- dir ( *string* ): Specify a new user directory, which only takes effect when the parameter isMove is true.

    This parameter cannot specify an existing directory. The specified new directory will retain the data of the original user directory, and the original user directory will be deleted.

    Format: `dir: "userHomeDir"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##EXAMPLES##

Modify the home directory of user `newUser` in the specified user group.

```lang-javascript
> System.setUserConfigs({name: "newUser", group: "groupName", dir: "/home/userName", isMove: true})
```



[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md