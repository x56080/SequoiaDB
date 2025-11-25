##NAME##

listLobPieces - list the LOB pieces in the collection

##SYNOPSIS##

**db.collectionspace.collection.listLobPieces()**

##CATEGORY##

Collection

##DESCRIPTION##

Users can use this function to get the LOB pieces in the collection, and the obtained result is returned by cursor.

##RETURN VALUE##

When the function executes successfully, it returns the DBCursor object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERROR##

The common exceptions of `listLobPieces()` function are as follows：

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist | Check whether the colletion space exists. |
| -23 | SDB_DMS_NOTEXIST| Collection does not exist | Check whether the colletion exists.|

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

The function is applicable to v5.8.4 and above.

##EXAMPLES##

* List all LOB pieces in sample.employee.

    ```lang-javascript
    > db.sample.employee.listLobPieces()
    {
      "Oid": {
        "$oid": "00006925cc8c3a00023465d8"
      },
      "Sequence": 1,
      "Length": 196387,
      "GroupID": 1000
    }
    {
      "Oid": {
        "$oid": "00006925cc8c3a00023465d8"
      },
      "Sequence": 0,
      "Length": 262144,
      "GroupID": 1000
    }
    {
      "Oid": {
        "$oid": "00006925cc983a00023465d9"
      },
      "Sequence": 1,
      "Length": 189915,
      "GroupID": 1000
    }
    {
      "Oid": {
        "$oid": "00006925cc983a00023465d9"
      },
      "Sequence": 0,
      "Length": 262144,
      "GroupID": 1000
    }
    Return 1 row(s).
    ```
