##NAME##

getIndex - get the specified index

##SYNOPSIS##

**db.collectionspace.collection.getIndex\(\<name\>\)**

##CATEGORY##

Collection

##DESCRIPTION##

Get the specified index from current collection.

##PARAMETERS##

* name ( *String*, *Required* )

Name of the specified index.

> **Note**
>
> * Index name should not contain null string, "." or "$". The length of it should not be greater than 127B.

##RETURN VALUE##

When the function executes successfully, it will return a specified index whose type is BSONObj.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `getIndex()`：

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-47       |SDB_IXM_NOTEXIST |Index doesn't exist | Check if the index exists|

when the exception happens, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v1.10 and above

##EXAMPLES##

* Get the index named ageIndex from the sample.employee collection.

   ```
   > db.sample.employee.getIndex( "ageIndex" )
   ```