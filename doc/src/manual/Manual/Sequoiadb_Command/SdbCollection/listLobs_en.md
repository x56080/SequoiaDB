##NAME##

listLobs - list the LOBs in the collection

##SYNOPSIS##

**db.collectionspace.collection.listLobs([SdbQueryOption])**

##CATEGORY##

Collection

##DESCRIPTION##

Users can use this function to get the LOBs in the collection, and the obtained result is returned by cursor.

##PARAMETERS##

SdbQueryOption( *Object*， *Optional* )
    
Use an object to specify record query parameters, and the usage can refer to [SdbQueryOption](reference/Sequoiadb_command/AuxiliaryObjects/SdbQueryOption.md).
    

>**Note:**
>
> Users can specify `SdbQueryOption.hint()` as follows:
>
> - `{"ListPieces": true}` to retrieve detailed pieces information of the LOB, which is equivalent to `listLobPieces()`.
> - `{"Oid":%oid_string%}` or `{"Oid":[%oid_string%, ...]}` to fetch specific LOB information, reducing I/O overhead.
> - `{"GroupID":%group_id%}` or `{"GroupID":[%group_id%,...]}` to retrieve LOB information for a specific Group, reducing I/O overhead.
>
> Users can also use `SdbQueryOption.cond()` with Oid/GroupID as an equality condition or $in filter (e.g., `{"Oid": {"$oid":%oid_string%}}`), which has the same effect as specifying Oid/GroupID in hint().


##RETURN VALUE##

When the function executes successfully, it returns the DBCursor object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERROR##

The common exceptions of `listLobs()` function are as follows：

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameter are filled in currectly. |
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist | Check whether the colletion space exists. |
| -23 | SDB_DMS_NOTEXIST| Collection does not exist | Check whether the colletion exists.|

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

The function is applicable to v2.0 and above, of which v3.2 and above support obtaining the specified LOB through input parameters.

##EXAMPLES##

* List all LOBs in sample.employee.

    ```lang-javascript
    > db.sample.employee.listLobs()
    {
       "Size": 2,
       "Oid": {
         "$oid": "00005d36c8a5350002de7edc"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.43.17.360000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.43.17.508000"
       },
       "Available": true,
       "HasPiecesInfo": false,
       "GroupID": 1001
     }
     {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false,
       "GroupID": 1001
    }
    Return 2 row(s).
    ```

* List LOBs with size greater than 10 in sample.employee.

    ```lang-javascript
    > db.sample.employee.listLobs( SdbQueryOption().cond( { "Size": { $gt: 10 } } ) )
    {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false,
       "GroupID": 1001
    }
    Return 1 row(s).
    ```

* List lob in sample.employee where Oid is "00005d36cae8370002de7edd".

    ```lang-javascript
    > db.sample.employee.listLobs( SdbQueryOption().hint( {Oid:"00005d36cae8370002de7edd"} ) )
    {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false,
       "GroupID": 1001
    }
    Return 1 row(s).
    ```
