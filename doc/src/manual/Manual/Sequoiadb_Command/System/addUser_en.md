
##NAME##

addUser - add an operating system user

##SYNOPSIS##

**System.addUser(\<User\>, [options])**

##CATEGORY##

System

##DESCRIPTION##

This function is used to add an operating system user. When adding a user through this function, root privileges are required.

##PARAMETERS##

The parameters are divided into those that can be specified when adding a user with a ciphertext password, and those that can be specified when adding a user with a plaintext password.

- User ( *object, required* )

    Specify the system username and password. The specific way can refer to the [User][User] object.

    > **Note:**
    >
    > Only SequoiaDB v3.4.4 and above support for specifying User objects.

- options ( *object, optional* )

    The user's attributes can be specified through the parameter "options":

    - gid ( *string* ): Specify the name or ID of the user primary group.

        This parameter must be specified as an existing user group. If not specified, a user group with the same name as the User object will be created by default.

        Format: `gid: "groupName"` or `gid: "2003"`

    - groups ( *string* ): Specify the name or ID list of the user supplementary groups.

        This parameter must be specified as an existing user group, each supplementary groups is separated by a comma.

        Format: `groups: "groupName1,groupName2,groupName3"` or `groups: "2004,2005,2006"`

    - createDir ( *boolean* ): Whether to create a user directory, the default is false.

        Format: `createDir: true`

    - dir ( *string* ): Specify the user directory, which takes effect only when the parameter "createDir" is true.

        This parameter cannot specify an existing directory. If not specified, a directory with the same name as the User object will be created in the `/home` directory as the user directory.

        Format: `dir: "userHomeDir"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2 and above

##EXAMPLES##

* Use the User object to add a new system user with name "newUser" and password "newUserPwd".

    ```lang-javascript
    > var u = new User("newUser", "newUserPwd")
    > System.addUser(u)
    ```

* Add a new system user with name "newUser" and specify the password through the interactive interface. At the same time, specify the user group as root, and create a user directory `/home/newUser`.

    ```lang-javascript
    > System.addUser(User("newUser").promptPassword(), {gid: "root", createDir: true, dir: "/home/newUser"})
    ```



[^_^]:
    links
[User]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md