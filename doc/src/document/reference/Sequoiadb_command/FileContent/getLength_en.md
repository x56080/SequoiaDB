##NAME##

getLength - Get the size of the binary stream.

##SYNOPSIS##

**getLength()**

##CATEGORY##

FileContent

##DESCRIPTION##

Get the size of the binary stream.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the size of the binary stream.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Open a banary file and get a file descriptor.

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.dump" )
```

* Read the contents of the file into the fileContent object( For more detail, please reference [File::readContent](reference/Sequoiadb_command/File/readContent) ).

```lang-javascript
> var content = file.readContent( 10000 )
```

* Get the size of the binary stream.

```lang-javascript
> content.getLength()
10000
```