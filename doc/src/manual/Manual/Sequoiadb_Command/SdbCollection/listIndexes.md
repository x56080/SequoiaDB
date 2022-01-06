##名称##

listIndexes - 列举集合的索引信息

##语法##

**db.collectionspace.collection.listIndexes\(\)**

##类别##

SdbCollection

##描述##

该函数用于列举指定集合的全部[索引][index]信息。用户通过协调节点执行该函数，将从编目节点获取索引信息；通过数据节点执行该函数，将从该数据节点获取索引信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取索引详细信息列表，字段说明如下：

| 字段名    | 类型  | 描述   | 
| ------    | --------  | ------ |
| name      | string    | 索引名 |
| key       | json    | 索引键，可参考 [indexDef][indexDef]        |
| v         | int32     | 索引版本号                                   |
| unique    | boolean   | 索引是否唯一 <br> "true"：该索引为唯一索引，不允许集合中有重复的值 <br> "false"：该索引为普通索引，允许集合中有重复的值                                     | 
| dropDups  | boolean   | 暂不开放                                     |
| enforced  | boolean   | 索引是否强制唯一，可参考 [enforced][enforced]          |
| NotNull   | boolean   | 索引的任意一个字段是否允许为 null 或者不存在 <br> "true"：不允许为 null 或者不存在 <br> "false"：允许为 null 或不存在    |
| IndexFlag | string    | 索引当前状态 <br> "Normal"：正常 <br> "Creating"：正在创建 <br> "Dropping"：正在删除 <br> "Truncating"：正在清空 <br> "Invalid"：无效                                                        |
| Type      | string    | 索引类型 <br> "Positive"：正序索引 <br> "Reverse"：逆序索引 <br> "Text"：全文索引                                       |
| NotArray  | boolean   | 索引的任意一个字段是否允许为数组 <br> "true"：不允许为数组 <br> "false"：允许为数组    |
| Standalone| boolean   | 是否为独立索引 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

返回集合 sample.employee 下的所有索引信息

```lang-javascript
> db.sample.employee.listIndexes()
{
  "_id": {
    "$oid": "6098e71a820799d22f1f2165"
  },
  "IndexDef": {
    "name": "$id",
    "_id": {
      "$oid": "6098e71a820799d22f1f2164"
    },
    "UniqueID": 4037269258240,
    "key": {
      "_id": 1
    },
    "v": 0,
    "unique": true,
    "dropDups": false,
    "enforced": true,
    "NotNull": false,
    "NotArray": true,
    "Global": false,
    "Standalone": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```

[^_^]:
     本文使用的所有引用及链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[indexDef]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[enforced]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
