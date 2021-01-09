[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见
    
    王涛：
    许建辉：
    市场部：20190425


集合快照： 

- 连接非协调节点，列出所有集合（不含临时的集合）
- 连接协调节点，列出所有集合（不含临时的集合和系统的集合）

标识
----

SDB_SNAP_COLLECTIONS

非协调节点字段信息
----

| 字段名           | 类型      | 描述                              |
| ---------------- | --------- | --------------------------------- |
| Name             | string    | 集合完整名                        |
| UniqueID         | int64     | 集合的 UniqueID，在集群上全局唯一 |
| CollectionSpace  | string    | 集合所属集合空间名                |
| Details.NodeName         | string    | 集合所属节点名，格式为<主机名>:<服务名> |
| Details.GroupName        | string    | 集合所属复制组名                  |
| Details.ID                      | int32         | 集合 ID，范围 0~4095，集合空间内唯一                    |
| Details.LogicalID               | int32         | 集合逻辑 ID                                             |
| Details.Sequence                | int32         | 序列号                                                  |
| Details.Indexes                 | int32         | 该集合所包含的索引数量                                  |
| Details.Status                  | string        | 集合当前状态，取值如下：<br>"Free"：空闲<br>"Normal"：正常<br>"Dropped"：被删除<br>"Offline Reorg Shadow Copy Phase"：离线重组复制阶段<br>"Offline Reorg Truncate Phase"：离线重组清除阶段<br>"Offline Reorg Copy Back Phase"：离线重组重入阶段<br>"Offline Reorg Rebuild Phase"：离线重组重建索引阶段 |
| Details.Attribute               | string        | 属性                                                    |
| Details.CompressionType         | string        | 压缩类型，如："snappy"、"lzw"                           |
| Details.DictionaryCreated       | boolean       | 是否创建压缩字典                                        |
| Details.DictionaryVersion       | int32         | 压缩字典版本                                            |
| Details.PageSize                | int32         | 集合页的大小                                            |
| Details.LobPageSize             | int32         | 大对象页的大小                                          |
| Details.TotalRecords            | int64         | 集合的记录总数                                          |
| Details.TotalLobs               | int64         | 集合的大对象总数                                        |
| Details.TotalDataPages          | int32         | 集合的数据页总数                                        |
| Details.TotalIndexPages         | int32         | 集合的索引页总数                                        |
| Details.TotalLobPages           | int32         | 集合的大对象页总数                                      |
| Details.TotalDataFreeSpace      | int64         | 集合的数据空闲空间，单位为字节                          |
| Details.TotalIndexFreeSpace     | int64         | 集合的索引空闲空间，单位为字节                          |
| Details.CurrentCompressionRatio | double        | 集合的的压缩率                                          |
| Details.DataCommitLSN           | int64         | 集合数据文件最后提交 LSN                                |
| Details.IndexCommitLSN          | int64         | 集合索引文件最后提交 LSN                                |
| Details.LobCommitLSN            | int64         | 集合大对象文件最后提交 LSN                              |
| Details.DataCommitted           | boolean       | 集合数据文件当前是否有效提交                            |
| Details.IndexCommitted          | boolean       | 集合索引文件当前是否有效提交                            |
| Details.LobCommitted            | boolean       | 集合大对象文件当前是否有效提交                          |
| Details.TotalDataRead   | int64 | 集合数据读请求 |
| Details.TotalIndexRead  | int64 | 集合索引读请求 |
| Details.TotalDataWrite  | int64 | 集合数据写请求 |
| Details.TotalIndexWrite | int64 | 集合索引写请求 |
| Details.TotalUpdate     | int64 | 集合更新记录数量 |
| Details.TotalDelete     | int64 | 集合删除记录数量 |
| Details.TotalInsert     | int64 | 集合插入记录数量 |
| Details.TotalSelect     | int64 | 集合选择记录数量 |
| Details.TotalRead       | int64 | 集合读取记录数量 |
| Details.TotalWrite      | int64 | 集合写入记录数量 |
| Details.TotalTbScan     | int64 | 集合使用表扫描次数 |
| Details.TotalIxScan     | int64 | 集合使用索引扫描次数 |
| Details.ResetTimestamp  | timestamp | 重置快照的时间 |

协调节点字段信息
----

| 字段名    | 类型      | 描述                               |
| --------- | --------- | ---------------------------------- |
| Name      | string    | 集合完整名                         |
| UniqueID  | int64     | 集合的 UniqueID，在集群上全局唯一  |
| Details.GroupName  | string    | 节点所在复制组名  |
| Details.Group.ID                  | int32         | 集合 ID，范围 0~4096，集合空间内唯一                    |
| Details.Group.LogicalID           | int32         | 集合逻辑 ID                                             |
| Details.Group.Sequence            | int32         | 序列号                                                  |
| Details.Group.Indexes             | int32         | 该集合所包含的索引数量                                  |
| Details.Group.Status              | string        | 集合当前状态，取值如下：<br>"Free"：空闲<br>"Normal"：正常<br>"Dropped"：被删除<br>"Offline Reorg Shadow Copy Phase"：离线重组复制阶段<br>"Offline Reorg Truncate Phase"：离线重组清除阶段<br>"Offline Reorg Copy Back Phase"：离线重组重入阶段<br>"Offline Reorg Rebuild Phase"：离线重组重建索引阶段 |
| Details.Group.TotalRecords        | int64         | 集合的记录总数                                          |
| Details.Group.TotalDataPages      | int32         | 集合的数据页总数                                        |
| Details.Group.TotalIndexPages     | int32         | 集合的索引页总数                                        |
| Details.Group.TotalLobPages       | int32         | 集合的大对象页总数                                      |
| Details.Group.TotalDataFreeSpace  | int64         | 集合的数据空闲空间，单位为字节                          |
| Details.Group.TotalIndexFreeSpace | int64         | 集合的索引空闲空间，单位为字节                          |
| Details.Group.TotalDataRead   | int64 | 集合数据读请求 |
| Details.Group.TotalIndexRead  | int64 | 集合索引读请求 |
| Details.Group.TotalDataWrite  | int64 | 集合数据写请求 |
| Details.Group.TotalIndexWrite | int64 | 集合索引写请求 |
| Details.Group.TotalUpdate     | int64 | 集合更新记录数量 |
| Details.Group.TotalDelete     | int64 | 集合删除记录数量 |
| Details.Group.TotalInsert     | int64 | 集合插入记录数量 |
| Details.Group.TotalSelect     | int64 | 集合选择记录数量 |
| Details.Group.TotalRead       | int64 | 集合读取记录数量 |
| Details.Group.TotalWrite      | int64 | 集合写入记录数量 |
| Details.Group.TotalTbScan     | int64 | 集合使用表扫描次数 |
| Details.Group.TotalIxScan     | int64 | 集合使用索引扫描次数 |
| Details.Group.ResetTimestamp  | timestamp | 重置快照的时间 |
| Details.Group.NodeName            | string        | 节点名，格式为<主机名>:<服务名>                         |


示例
----

- 通过非协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_COLLECTIONS )
   ```
   
   输出结果如下：

   ```lang-json
   {
     "Name": "sample.employee",
     "UniqueID": 261993005057,
     "CollectionSpace": "sample",
     "Details": [
       {
         "NodeName": "hostname:11890",
         "GroupName": "group1",
         "ID": 0,
         "LogicalID": 0,
         "Sequence": 1,
         "Indexes": 8,
         "Status": "Normal",
         "Attribute": "",
         "CompressionType": "",
         "DictionaryCreated": false,
         "DictionaryVersion": 0,
         "PageSize": 65536,
         "LobPageSize": 262144,
         "TotalRecords": 0,
         "TotalLobs": 0,
         "TotalDataPages": 0,
         "TotalIndexPages": 6,
         "TotalLobPages": 0,
         "TotalDataFreeSpace": 0,
         "TotalIndexFreeSpace": 196545,
         "CurrentCompressionRatio": 1,
         "DataCommitLSN": 160,
         "IndexCommitLSN": 180,
         "LobCommitLSN": -1,
         "DataCommitted": false,
         "IndexCommitted": false,
         "LobCommitted": true,
         "TotalDataRead": 0,
         "TotalIndexRead": 0,
         "TotalDataWrite": 100,
         "TotalIndexWrite": 100,
         "TotalUpdate": 0,
         "TotalDelete": 0,
         "TotalInsert": 100,
         "TotalSelect": 0,
         "TotalRead": 0,
         "TotalWrite": 100,
         "TotalTbScan": 0,
         "TotalIxScan": 0,
         "ResetTimestamp": "2019-06-19-17.46.32.867539"
       }
     ]
   }
   ...
   ```

- 通过协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_COLLECTIONS )
   ```

   输出结果如下：

   ```lang-json
   {
     "Name": "sample.employee",
     "UniqueID": 261993005058,
     "Details": [
       {
         "GroupName": "group1",
         "Group": [
           {
             "ID": 0,
             "LogicalID": 0,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 1,
             "TotalDataPages": 1,
             "TotalIndexPages": 2,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 4004,
             "TotalIndexFreeSpace": 4046,
             "TotalDataRead": 0,
             "TotalIndexRead": 0,
             "TotalDataWrite": 100,
             "TotalIndexWrite": 100,
             "TotalUpdate": 0,
             "TotalDelete": 0,
             "TotalInsert": 100,
             "TotalSelect": 0,
             "TotalRead": 0,
             "TotalWrite": 100,
             "TotalTbScan": 0,
             "TotalIxScan": 0,
             "ResetTimestamp": "2019-06-19-17.46.32.867539",
             "NodeName": "hostname:11820"
           }
         ]
       }
     ]
   }
   ...
   ```