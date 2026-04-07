##NAME##

reset - Reset the parameters in the DiagLog object

##SYNOPSIS##

**diaglog.collect().reset()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Reset the parameters in the DiagLog object.

##PARAMETERS##

NULL

##RETURN VALUE##

NULL

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog()
    ```

* Reset the parameters in the DiagLog object.

    ```lang-javascript
    > diaglog.reset()
    ```
