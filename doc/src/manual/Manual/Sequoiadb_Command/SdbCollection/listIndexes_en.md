##NAME##

listIndexes - enumerate the index information under the collection

##SYNOPSIS##

**db.collectionspace.collection.listIndexes\(\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to enumerate [index][index]. Executing this method will display all the index information under the specified collection.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of index details through this object, and the field descriptions are as follows:

| Name    | Type  |Description   | 
| ------    | --------  | ------ |
| name      | string    | Index name |
| key       | json | Index key, refer to [indexDef][indexDef]       |
| v         | int32     | Index version number                                 |
| unique    | boolean   | Whether the index is unique.<br> "true": The index is a unique index, and duplicate values are not allowed in the collection.<br> "false": The index is a normal index, and duplicate values are allowed in the collection.                                     | 
| dropDups  | boolean   | Not open                                    |
| enforced  | boolean   | Whether the index is mandatory to be unique, refer to [enforced][enforced].          |
| NotNull   | boolean   | Whether any field of the index is allowed to be "null" or does not exist. <br> "true": Not allowed to be "null" or not exist. <br> "false": Allow null or not exist.    |
| IndexFlag | string    | Current state of Index <br> "Normal": Normal <br> "Creating": Creating <br> "Dropping": Dropping <br> "Truncating": Emptying <br> "Invalid": Invalid                                                        |
| Type      | string    | Index type <br> "Positive": Positive index <br> "Reverse": Reverse index <br> "Text": Full-text index                                       |
| NotArray| boolean   | Whether any field of the index is allowed to be an array. <br> "true": Not allowed to be an array. <br> "false": Allowed as an array.    |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

#EXAMPLES##

Return all index information under the collection "sample.employee".

```lang-javascript
> db.sample.employee.listIndexes()
{
  "_id": {
    "$oid": "6098e71a820799d22f1f2165"
  },
  "IndexDef": {
    "name": "$id",
    "_id": {
      "$oid": "6098e71a820799d22f1f2164"
    },
    "UniqueID": 4037269258240,
    "key": {
      "_id": 1
    },
    "v": 0,
    "unique": true,
    "dropDups": false,
    "enforced": true,
    "NotNull": false,
    "NotArray": true,
    "Global": false,
    "Standalone": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[indexDef]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[enforced]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
