##NAME##

listIndexes - list the index information in the collection

##SYNOPSIS##

**db.collectionspace.collection.listIndexes\(\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to list the information of all [indexes]((basic_operation/indexes.md)) in the specified collection.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of index details through this object, the field descriptions are as follows:

| Name    | Type  | Description   | 
| ------    | --------  | ------ |
| name      | string    | Index name |
| key       | json    | Index key, the value is as follows:<br>1: Ascending by field<br>-1: Descending order by field<br>"text": [Full-text index](basic_operation/text_search/overview.md)        |
| v         | int32     | Index version number                                   |
| unique    | boolean   | Is the index unique, the value is as follows:<br> "true": Unique index, no duplicate values in the collection are allowed.<br> "false": Ordinary index, allowing duplicate values in the collection.                                   | 
| enforced  | boolean   | Whether the index is mandatory to be unique, the value is as follows:<br>"false": Not mandatory.<br>"true": Mandatory unique, which means that more than one empty index key is not allowed.      |
| NotNull   | boolean   | Whether any field of the index is allowed to be "null" or non-existent, the value is as follows: <br> "true": Not allowed to be "null" or non-existent. <br> "false": Allow "null" or not exist.    |
| IndexFlag | string    | Index current state, the value is as follows: <br> "Normal": Normal <br> "Creating": Creating <br> "Dropping": Dropping <br> "Truncating": Truncating <br> "Invalid": Invalid                                                       |
| Type      | string    | Index type, the value is as follows:<br> "Positive": Positive index <br> "Reverse": Reverse index <br> "Text": Full-text index                                     |
| NotArray| boolean   | Whether any field of the index is allowed to be an array, the value is as follows:<br> "true": Not allowed to be an array. <br> "false": Allowed as an array.    |
| dropDups  | boolean   | Not open                                  |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

List the information of all indexes in the collection "sample.employee".

```lang-javascript
> db.sample.employee.listIndexes()
{
  "IndexDef": {
    "name": "$id",
    "_id": {
      "$oid": "5e9e91bccf4f1e7370e4074d"
    },
    "key": {
      "_id": 1
    },
    "v": 0,
    "unique": true,
    "dropDups": false,
    "enforced": true,
    "NotNull": false，
    "NotArray": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```