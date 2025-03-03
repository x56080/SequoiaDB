##NAME##

lobCount - count the total number of Lobs in the current collection

##SYNOPSIS##

**db.collectionspace.collection.lobCount([cond])**

**db.collectionspace.collection.lobCount([cond]).hint([hint])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to count the total number of lobs in the current collection, users can specify the control parameters through hint.

##PARAMETERS##

The usage of the parameters `cond` is used in the same way as [find()][find], and the `hint` is used in the same way as [listLobs()][listLobs].

##RETURN VALUE##

When the function executes successfully, it will return an object of type CLCount. Users can get the total number of records that meet the conditions through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `count()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameters are filled in correctly.|
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist.| Check whether the collection space exists.|
| -23 | SDB_DMS_NOTEXIST| Collection does not exist. | Check whether the collection exists.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.14/v5.8.4 and above

##EXAMPLES##

- Count the number of lobs in the collection "sample.lob", without specifying the parameter "cond".

    ```lang-javascript
    db.sample.lob.lobCount()
    ```
- Count the number of LOBs that meet the conditions and have a size greater than 1MB.

    ```lang-javascript
    > db.sample.lob.lobCount({Size: {$gt:1024*1024*1024}})
    ```

- Count the number of LOB pieces that meet the condition where Oid is "000067c20f5232000457d32a".

    ```lang-javascript
    > db.sample.lob.lobCount().hint({Oid:"000067c20f5232000457d32a",ListPieces:true})
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[listLobs]:manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md