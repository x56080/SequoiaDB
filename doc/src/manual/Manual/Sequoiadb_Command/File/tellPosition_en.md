
##NAME##

tellPositon - Return the offset of file position.

##SYNOPSIS##

***File.tellPositon()***

##CATEGORY##

File

##DESCRIPTION##

Return the offset of file position.

##PARAMETERS##

NA

##RETURN VALUE##

On success, return the offset of file position.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open a file and get the offset of file position.

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file.txt" )
    > file.tellPosition()
    0
    ```

* Write content, and get the offset of file position.

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file.txt" )
    > file.write( "sample" )
    > file.tellPosition()
    6
    ```

* Moving cursor, and get the offset of file position.

    ```lang-javascript
    > file.seek(2)
    > file.tellPosition()
    2
    ```

