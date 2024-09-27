##NAME##

start - execute the shell command in the background

##SYNOPSIS##

**cmd.start(\<cmd\>, [args], [useShell], [timeout])**

##CATEGORY##

Cmd

##DESCRIPTION##

Execute the Shell command in the background.

##PARAMETERS##

| Name     | Type     | Default | Description        | Required or not |
| -------- | -------- | ------- | ------------------ | --------------- |
| cmd      | string   | ---     | Shell command name | Required        |
| args     | string   | NULL    | Command parameter  | Not             |
| useShell | number   | 1       | whether to use /bin/sh to parse and execute the command. Default use /bin/sh.  | Not             |
| timeout  | number   | 0       | Set timeout        | Not             |


##RETURN VALUE##

On success, return the pid of the command execution.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##VERSION##

v3.2 and above

##EXAMPLES##

* Create a Command object.

    ```lang-javascript
    > var cmd = new Cmd()
    ```

* Execute the command.

    ```lang-javascript
    > cmd.start( "ls", "/opt/trunk/test" )
    28340
    ```

* Get the result of the command execution.

    ```lang-javascript
    > cmd.getLastOut()
    test1
    test2
    test3 
    ```
