##NAME##

run - Run with the currently set parameters

##SYNOPSIS##

**diaglog.run()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Run with the currently set parameters.

##PARAMETERS##

NULL

##RETURN VALUE##

The result of the execution.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* After setting the parameters, run the program.

    ```lang-javascript
    > var filename = diaglog.search().error( -79 ).limit( 10 ).run();
    ```
