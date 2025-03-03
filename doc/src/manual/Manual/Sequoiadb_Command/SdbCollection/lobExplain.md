##名称##

lobExplain - 获取大对象在集合中分片位置信息的执行计划

##语法##

**db.collectionspace.collection.lobExplain(\<oid\>, [detail], [options])**

##类别##

SdbCollection

##描述##

该函数用于获取大对象在集合中分片位置信息的执行计划。

##参数##

| 参数名    | 参数类型 | 描述   | 是否必填 |
| --------- | -------- | ------ | -------- |
| oid       | string   | 大对象的唯一描述符。              | 是 |
| detail    | boolean  | 位置信息中是否显示详细分片号      | 否 |
| options   | object   | 扩展选项，Offset:大对象计算的起始位置，Length：大对象计算的长度。 | 否 |

  > **Note**
  >
  > - 当大对象存在时，`Offset` 默认为 0，`Length` 默认为大对象的大小；如果大对象大小大于2GB时，会按2GB进行计算。可以通过指定`Offset` 和 `Length` 进行控制。
  > - 当大对象不存在时，`Offset` 默认为 0，`Length` 默认为 512KB。
  > - 指定 `Length` 超过 2GB(2147483648) 时，会调整为 2GB。

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取大对象在集合中分片位置信息的执行计划。

函数执行失败时，将抛异常并输出错误信息。

大对象被读写访问的详细信息格式为：

|字段名     |描述                  |
| --------- | --------             |
|Oid        |大对象的唯一描述符。  |
|LobPageSize|LOB页大小。           |
|Exist      |大对象是否已存在。    |
|Offset     |大对象计算的起始位置。|
|Length     |大对象计算的长度。    |
|GroupID    |元数据分片所在的数据组ID。|
|Location   |大对象分片位置信息。数组对象类型|
|PiecesNum  |大对象分片数量。      |
|SubCLName  |大对象所在子集合名。（仅当从主表执行操作才显示）|

其中 Location 数组对象的详细信息格式为：

|字段名     |描述                  | 说明 |
| --------- | --------             | ---  |
|GroupID    | 数据组ID             |      |
|PiecesNum  | 位于该数据组的分片数量|     |
|Pieces     | 分片ID数组           | 仅当 detail=true 时显示|


##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##示例##

获取 000067bce21e330004538d02 的 lob 分片执行计划

```lang-javascript
 > db.sample.lob.lobExplain('000067bce21e330004538d02')
 {
   "Oid": "000067bce21e330004538d02",
   "LobPageSize": 262144,
   "Exist": true,
   "Offset": 0,
   "Length": 2597114,
   "GroupID": 1001,
   "Location": [
     {
       "GroupID": 1000,
       "PiecesNum": 3
     },
     {
       "GroupID": 1001,
       "PiecesNum": 6
     },
     {
       "GroupID": 1006,
       "PiecesNum": 1
     }
   ],
   "PiecesNum": 10
 }
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
