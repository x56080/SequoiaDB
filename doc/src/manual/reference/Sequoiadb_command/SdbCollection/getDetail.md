##名称##

getDetail - 获取集合具体信息

##语法##

**db.collectionspace.collection.getDetail\(\)**

##描述##

该函数用于获取当前集合在各个数据组的具体信息。

##参数##

无

##返回值##

函数执行成功时，通过游标（cursor）的方式返回集合详细信息列表。返回的字段信息可参考[集合快照视图](reference/SQL_grammar/monitoring/SNAPSHOT_CL.md)。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](manual/faq.md)。 

##版本##

v3.2.5 及以上版本、v3.4.1 及以上版本

##示例##

* 获取集合 `sample.employee` 的详细信息

 ```lang-javascript
 > db.sample.employee.getDetail()
 {
   "Name": "sample.employee",
   "UniqueID": 22574348107782,
   "CollectionSpace": "sample",
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
