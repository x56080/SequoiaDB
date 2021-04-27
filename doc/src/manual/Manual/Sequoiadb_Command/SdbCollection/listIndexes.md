## 名称

listIndexes - 枚举集合下的索引信息

## 语法

**db.collectionspace.collection.listIndexes\(\)**

## 类别

SdbCollection

## 描述

该函数用于枚举[索引](manual/Distributed_Engine/Architecture/Data_Model/index.md)，执行此方法会将指定集合下的索引信息全部显示出来。

## 参数

无

## 字段信息

| 参数名    | 参数类型  | 描述   | 
| ------    | --------  | ------ |
| name      | string    | 索引名 |
| key       | json 对象 | 索引键，可参考 [indexDef](manual/Manual/Sequoiadb_Command/SdbCollection/createIdIndex.md)           |
| v         | Int32     | 索引版本号                                   |
| unique    | Boolean   | 索引是否唯一 <br> "true"：该索引为唯一索引，不允许集合中有重复的值 <br> "false"：该索引为普通索引，允许集合中有重复的值                                     | 
| dropDups  | Boolean   | 暂不开放                                     |
| enforced  | Boolean   | 索引是否强制唯一，可参考 [enforced](manual/Manual/Sequoiadb_Command/SdbCollection/createIdIndex.md)           |
| NotNull   | Boolean   | 索引的任意一个字段是否允许为 null 或者不存在 <br> "true"：不允许为 null 或者不存在 <br> "false"：允许为 null 或不存在    |
| IndexFlag | string    | 索引当前状态 <br> "Normal"：正常 <br> "Creating"：正在创建 <br> "Dropping"：正在删除 <br> "Truncating"：正在清空 <br> "Invalid"：无效                                                        |
| Type      | string    | 索引类型 <br> "Positive"：正序索引 <br> "Reverse"：逆序索引 <br> "Text"：全文索引                                       |
| NotArray| Boolean   | 索引的任意一个字段是否允许为数组 <br> "true"：不允许为数组 <br> "false"：允许为数组    |

## 返回值

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取集合详细信息列表，字段说明可参考[索引统计信息快照][SDB_SNAP_INDEXSTATS]。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 示例

* 返回集合 employee 下的所有索引信息

```lang-javascript
> db.sample.employee.listIndexes()
{
  "IndexDef": {
    "name": "$id",
    "_id": {
      "$oid": "5e9e91bccf4f1e7370e4074d"
    },
    "key": {
      "_id": 1
    },
    "v": 0,
    "unique": true,
    "dropDups": false,
    "enforced": true,
    "NotNull": false，
    "NotArray": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md

[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md