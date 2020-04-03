##语法##
***db.collectionspace.collection.getDetail\(\)***

获取当前集合具体信息。

##返回值##

获取集合详细信息列表，并通过游标（cursor）的方式返回。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

## 示例##

* 获取集合 `foo.bar` 的详细信息。字段具体含义请参考[集合快照视图](reference/SQL_grammar/monitoring/SNAPSHOT_CL.md)

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
