
##描述##

集合快照 $SNAPSHOT_CL 列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标识##

$SNAPSHOT_CL

##字段信息##

| 字段名              | 类型          | 描述                                                    |
| ------------------- | ------------- | ------------------------------------------------------- |
| Name                | string        | 集合完整名                                              |
| UniqueID            | int64         | 集合的 UniqueID，在集群上全局唯一                        |
| CollectionSpace     | string        | 集合所属集合空间名                                      |
| Details.NodeName            | string        | 集合所属节点名，格式为<主机名>:<端口号>                        |
| Details.GroupName           | string        | 集合所属复制组名                                        |
| Details.InternalV           | int32         | 集合视图的版本                                  |
| Details.ID          | int32         | 集合 ID，范围 0 ~ 4095，集合空间内唯一                  |
| Details.LogicalID   | int32         | 集合逻辑 ID                                             |
| Details.Sequence    | int32         | 序列号                                                  |
| Details.Indexes     | int32         | 该集合所包含的索引数量                                  |
| Details.Status      | string        | 集合当前状态，取值如下：<br> "Free"：空闲<br>"Normal"：正常<br>"Dropped"：被删除<br> "Offline Reorg Shadow Copy Phase"：离线重组复制阶段<br> "Offline Reorg Truncate Phase"：离线重组清除阶段<br> "Offline Reorg Copy Back Phase"：离线重组重入阶段<br> "Offline Reorg Rebuild Phase"：离线重组重建索引阶段 |
| Details.Attribute   |  string       | 属性 |
| Details.CompressionType | string    | 压缩类型 |
| Details.DictionaryCreated | boolean | 是否创建压缩字典  |
| Details.DictionaryVersion   | int32 | 压缩字典版本 |
| Details.PageSize            | int32 | 集合页的大小  |
| Details.LobPageSize         | int32 | 大对象页的大小   |
| Details.TotalRecords        | int64         | 集合的记录总数                                          |
| Details.TotalLobs           | int64         | 集合的大对象总数  |
| Details.TotalDataPages      | int32         | 集合的数据页总数                                        |
| Details.TotalIndexPages     | int32         | 集合的索引页总数                                        |
| Details.TotalLobPages       | int32         | 集合的大对象页总数                                      |
| Details.TotalDataFreeSpace  | int64         | 集合的数据空闲空间，单位为字节                        |
| Details.TotalIndexFreeSpace | int64         | 集合的索引空闲空间，单位为字节                        |
| Details.CurrentCompressionRatio | double    | 集合的的压缩率                                          |
| Details.DataCommitLSN   | int64      | 集合数据文件最后提交LSN    |
| Details.IndexCommitLSN  | int64      | 集合索引文件最后提交LSN    |
| Details.LobCommitLSN    | int64      | 集合大对象文件最后提交LSN  |
| Details.DataCommitted   | boolean    | 集合数据文件当前是否有效提交 |
| Details.IndexCommitted  | boolean    | 集合索引文件当前是否有效提交 |
| Details.LobCommitted    | boolean    | 集合大对象文件当前是否有效提交 |
| Details.TotalDataRead   | int64  | 集合数据读请求 |
| Details.TotalIndexRead  | int64  | 集合索引读请求 |
| Details.TotalDataWrite  | int64  | 集合数据写请求 |
| Details.TotalIndexWrite | int64  | 集合索引写请求 |
| Details.TotalUpdate     | int64  | 集合更新记录数量 |
| Details.TotalDelete     | int64  | 集合删除记录数量 |
| Details.TotalInsert     | int64  | 集合插入记录数量 |
| Details.TotalSelect     | int64  | 集合选择记录数量 |
| Details.TotalRead       | int64  | 集合读取记录数量 |
| Details.TotalWrite      | int64  | 集合写入记录数量 |
| Details.TotalTbScan     | int64  | 集合使用表扫描次数 |
| Details.TotalIxScan     | int64  | 集合使用索引扫描次数 |
| Details.ResetTimestamp  | timestamp | 重置快照的时间 |

##示例##

查看集合快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CL" )
```

输出结果如下：

```lang-json
{
  "Name": "newcs.newcl",
  "UniqueID": 4294967297,
  "CollectionSpace": "newcs",
  "Details": [
    {
      "NodeName": "hostname:20000",
      "GroupName": "db1",
      "InternalV": 1,
      "ID": 0,
      "LogicalID": 0,
      "Sequence": 1,
      "Indexes": 2,
      "Status": "Normal",
      "Attribute": "Compressed",
      "CompressionType": "lzw",
      "DictionaryCreated": true,
      "DictionaryVersion": 1,
      "PageSize": 65536,
      "LobPageSize": 262144,
      "TotalRecords": 1052000,
      "TotalLobs": 0,
      "TotalDataPages": 1259,
      "TotalIndexPages": 1130,
      "TotalLobPages": 0,
      "TotalDataFreeSpace": 38324,
      "TotalIndexFreeSpace": 17092920,
      "CurrentCompressionRatio": 0.3,
      "DataCommitLSN": 132007488,
      "IndexCommitLSN": 2184160,
      "LobCommitLSN": 0,
      "DataCommitted": true,
      "IndexCommitted": true,
      "LobCommitted": true,
      "TotalDataRead": 0,
      "TotalIndexRead": 0,
      "TotalDataWrite": 1052000,
      "TotalIndexWrite": 2078000,
      "TotalUpdate": 0,
      "TotalDelete": 0,
      "TotalInsert": 1052000,
      "TotalSelect": 0,
      "TotalRead": 0,
      "TotalWrite": 1052000,
      "TotalTbScan": 0,
      "TotalIxScan": 6,
      "ResetTimestamp": "2019-06-24-17.12.30.279933"
    }
  ]
}
...
```
