##NAME##

getIndex - get the specified index

##SYNOPSIS##

**db.collectionspace.collection.getIndex(\<name\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to get the specified index from current collection.

##PARAMETERS##

name ( *string, required* )

Index name, the length cannot exceed 127B, and cannot be an empty string, dot(.) or dollar sign($).

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get a list of collection details through this object. For field descriptions, refer to [SDB_SNAP_INDEXSTATS][SDB_SNAP_INDEXSTATS].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `getIndex()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
|-47       |SDB_IXM_NOTEXIST |Index doesn't exist | Check if the index exists|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.10 and above

##EXAMPLES##

Get the index named ageIndex from the "sample.employee" collection.

```lang-javascript
> db.sample.employee.getIndex("ageIndex")
```

The result is as follows:

```lang-json
{
 "IndexDef": {
     "name": "ageIndex",
     "_id": {
       "$oid": "5f4f3b938f5a48a0c3a5f3ad"
     },
     "key": {
       "age": 1
     },
     "v": 0,
    "unique": true,
     "dropDups": false,
     "enforced": false,
     "NotNull": false,
     "NotArray": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```

[^_^]:
    links
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md

