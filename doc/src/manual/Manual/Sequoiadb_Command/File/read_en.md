
##NAME##

read - read file

##SYNOPSIS##

**File.read(\[size\])**

##CATEGORY##

File

##DESCRIPTION##

This function is used to read the specified text file.

##PARAMETERS##

size ( *number, optional* )

Starting from the current file cursor position, the number of bytes to be read, and the default value is 1024.

##RETURN VALUE##

When the function executes successfully, it will return the read file content.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2 and above

##EXAMPLSES##

- Read 1024 bytes from file `file` starting at the cursor position.

    ```lang-javascript
    > var file = new File("/opt/sequoiadb/file")
    > file.read()
    ```

- Used with [getSize][getSize] to read all content after the cursor position of file `file`.

    ```lang-javascript
    > var file = new File("/opt/sequoiadb/file")
    > file.read(file.getSize("/opt/sequoiadb/file"))
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getSize]:manual/Manual/Sequoiadb_Command/File/getSize.md