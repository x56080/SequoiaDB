[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见
    
    王涛：
    许建辉：
    市场部：20190425


集合空间快照可以列出所有集合空间。用户通过协调节点或非协调节点查看该快照时，返回的结果字段不完全相同。


标识
----

SDB_SNAP_COLLECTIONSPACES

非协调节点字段信息
----

| 字段名          | 类型       | 描述                                         |
| --------------- | ---------- | -------------------------------------------- |
| NodeName        | string     | 集合空间所属节点名，格式为<主机名>:<服务名>  |
| GroupName       | string     | 集合空间所属分区组名                         |
| Name            | string     | 集合空间名                                   |
| UniqueID        | int32      | 集合空间的 UniqueID，在集群上全局唯一        |
| ID              | int32      | 集合空间在节点上的物理 ID                     |
| LogicalID       | int32      | 集合空间在节点上的逻辑 ID                     |
| Collection.Name | string     | 集合空间所包含的集合的名字                   |
| Collection.UniqueID | int64  | 集合空间所包含的集合的 UniqueID               |
| PageSize        | int32      | 集合空间数据页大小，单位为字节               |
| LobPageSize     | int32      | 集合空间大对象数据页大小，单位为字节         |
| MaxCapacitySize | int64      | 集合空间的最大容量上限，单位为字节           |
| MaxDataCapSize  | int64      | 集合空间数据文件最大容量上限，单位为字节     |
| MaxIndexCapSize | int64      | 集合空间索引文件最大容量上限，单位为字节     |
| MaxLobCapSize   | int64      | 集合空间大对象文件最大容量上限，单位为字节   |
| NumCollections  | int32      | 集合数量                                     |
| TotalRecords    | int32      | 集合空间的记录总数                           |
| TotalSize       | int64      | 集合空间的总大小，单位为字节                 |
| FreeSize        | int64      | 集合空间的空闲大小，单位为字节               |
| TotalDataSize   | int64      | 集合空间数据文件总大小，单位为字节           |
| FreeDataSize    | int64      | 集合空间数据文件空闲空间大小，单位为字节     |
| TotalIndexSize  | int64      | 集合空间索引文件总大小，单位为字节           |
| FreeIndexSize   | int64      | 集合空间索引文件空闲空间大小，单位为字节     |
| TotalLobSize    | int64      | 集合空间大对象文件总大小，单位为字节         |
| FreeLobSize     | int64      | 集合空间大对象文件空闲空间大小，单位为字节   |
| DataCommitLSN   | int64      | 集合空间数据文件最后提交 LSN                 |
| IndexCommitLSN  | int64      | 集合空间索引文件最后提交 LSN                 |
| LobCommitLSN    | int64      | 集合空间大对象文件最后提交 LSN               |
| DataCommitted   | boolean    | 集合空间数据文件当前是否有效提交             |
| IndexCommitted  | boolean    | 集合空间索引文件当前是否有效提交             |
| LobCommitted    | boolean    | 集合空间大对象文件当前是否有效提交           |
| DirtyPage       | int32      | 集合空间大对象文件在开启缓存下脏页数量       |
| Type            | int32      | 集合空间类型，取值如下：<br>0：普通集合空间<br>1：固定（Capped）集合空间 |

协调节点字段信息
----

| 字段名          | 类型       | 描述                                         |
| --------------- | ---------- | -------------------------------------------- |
| Name            | string     | 集合空间名                                   |
| UniqueID        | int32      | 集合空间的 UniqueID，在集群上全局唯一        |
| Collection.Name | string     | 集合空间所包含的集合的名字                   |
| Collection.UniqueID | int64 | 集合空间所包含的集合的 UniqueID               |
| PageSize        | int32      | 集合空间数据页大小，单位为字节               |
| LobPageSize     | int32      | 集合空间大对象数据页大小，单位为字节         |
| TotalSize       | int64      | 集合空间的总大小，单位为字节                 |
| FreeSize        | int64      | 集合空间的空闲大小，单位为字节               |
| TotalDataSize   | int64      | 集合空间数据文件总大小，单位为字节           |
| FreeDataSize    | int64      | 集合空间数据文件空闲空间大小，单位为字节     |
| TotalIndexSize  | int64      | 集合空间索引文件总大小，单位为字节           |
| FreeIndexSize   | int64      | 集合空间索引文件空闲空间大小，单位为字节     |
| TotalLobSize    | int64      | 集合空间大对象文件总大小，单位为字节         |
| FreeLobSize     | int64      | 集合空间大对象文件空闲空间大小，单位为字节   |
| Group           | bson array | 该集合空间所在的复制组名列表                 |

示例
----

- 通过非协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_COLLECTIONSPACES )
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver1:11830",
     "GroupName": "group1",
     "Name": "sample",
     "UniqueID": 61,
     "ID" : 20,
     "LogicalID" : 20,
     "Collection": [
       {
         "Name": "employee",
         "UniqueID": 261993005057
       }
     ],
     "PageSize": 65536,
     "LobPageSize": 262144,
     "MaxCapacitySize": 26388279066624,  
     "MaxDataCapSize": 8796093022208,
     "MaxIndexCapSize": 8796093022208,
     "MaxLobCapSize": 8796093022208,
     "NumCollections": 4,
     "TotalRecords": 2,
     "TotalSize": 306315264,
     "FreeSize": 265551224,
     "TotalDataSize": 155254784,
     "FreeDataSize": 133627904,
     "TotalIndexSize": 151060480,
     "FreeIndexSize": 134152171,
     "TotalLobSize": 352714752,
     "FreeLobSize": 140771328,
     "DataCommitLSN": 160,
     "IndexCommitLSN": 180,
     "LobCommitLSN": -1,
     "DataCommitted": false,
     "IndexCommitted": false,
     "LobCommitted": true,
     "DirtyPage": 0
   }
   ...
   ```

- 通过协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_COLLECTIONSPACES )
   ```
   
   输出结果如下：

   ```lang-json
   {
     "Name": "sample",
     "UniqueID": 61,
     "PageSize": 4096,  
     "LobPageSize": 262144,
     "TotalSize": 918945792,
     "FreeSize": 805183062,  
     "TotalDataSize": 155254784,
     "FreeDataSize": 133627904,
     "TotalIndexSize": 151060480,
     "FreeIndexSize": 134152171,
     "TotalLobSize": 352714752,
     "FreeLobSize": 140771328,
     "Collection": [
       {
         "Name": "employee",
         "UniqueID": 261993005057
       }
     ],
     "Group": [
       "group1"
     ]
   }
   ...
   ```