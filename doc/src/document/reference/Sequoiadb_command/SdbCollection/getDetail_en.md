##NAME##

getDetail - Get the detail information of current collection.

##SYNOPSIS##

***db.collectionspace.collection.getDetail\(\)***

##CATEGORY##

SdbCollection

##DESCRIPTION##

Get the detail information of current collection.

##PARAMETERS##

##RETURN VALUE##

On success, return the detail information of collection.

On error, exception will be thrown.

##ERRORS##

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##EXAMPLES##

* Get the detail of collection `foo.bar`.

 ```lang-javascript
 > db.foo.bar.getDetail()
 {
   "Name": "foo.bar",
   "UniqueID": 22574348107782,
   "CollectionSpace": "foo",
   "Details": [
     {
       "NodeName": "hostname:11920",
       "GroupName": "group1",
       "ID": 1,
       "LogicalID": 1,
       "Sequence": 1,
       "Indexes": 2,
       "Status": "Normal",
       "Attribute": "Compressed",
       "CompressionType": "lzw",
       "DictionaryCreated": false,
       "DictionaryVersion": 0,
       "PageSize": 65536,
       "LobPageSize": 262144,
       "TotalRecords": 0,
       "TotalLobs": 0,
       "TotalDataPages": 0,
       "TotalIndexPages": 4,
       "TotalLobPages": 0,
       "TotalDataFreeSpace": 0,
       "TotalIndexFreeSpace": 131030,
       "CurrentCompressionRatio": 1,
       "DataCommitLSN": 14295548352,
       "IndexCommitLSN": 14295548428,
       "LobCommitLSN": 0,
       "DataCommitted": true,
       "IndexCommitted": true,
       "LobCommitted": true,
       "TotalDataRead": 0,
       "TotalIndexRead": 0,
       "TotalDataWrite": 0,
       "TotalIndexWrite": 0,
       "TotalUpdate": 0,
       "TotalDelete": 0,
       "TotalInsert": 0,
       "TotalSelect": 0,
       "TotalRead": 0,
       "TotalWrite": 0,
       "TotalTbScan": 0,
       "TotalIxScan": 0,
       "ResetTimestamp": "2020-04-04-16.20.52.155061"
     }
   ]
 }
 ...
 ```


