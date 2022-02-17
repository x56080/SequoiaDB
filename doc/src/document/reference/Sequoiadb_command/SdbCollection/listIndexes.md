##名称##

listIndexes - 列举集合中的索引信息

##语法##

**db.collectionspace.collection.listIndexes\(\)**

##类别##

SdbCollection

##描述##

该函数用于列举指定集合中所有[索引](basic_operation/indexes.md)的信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取索引详细信息列表，字段说明如下：

| 字段名    | 类型  | 描述   | 
| ------    | --------  | ------ |
| name      | string    | 索引名 |
| key       | json    | 索引键，取值如下：<br>1：按字段升序<br>-1：按字段降序<br>"text"：[全文索引](basic_operation/text_search/overview.md)       |
| v         | int32     | 索引版本号                                   |
| unique    | boolean   | 索引是否唯一，取值如下： <br> "true"：唯一索引，不允许集合中有重复的值 <br> "false"：普通索引，允许集合中有重复的值                                     | 
| enforced  | boolean   | 索引是否强制唯一，取值如下：<br>"false"：不强制唯一<br>"true"：强制唯一，即不允许存在一个以上全空的索引键       |
| NotNull   | boolean   | 索引的任意一个字段是否允许为 null 或者不存在，取值如下： <br> "true"：不允许为 null 或者不存在 <br> "false"：允许为 null 或不存在    |
| IndexFlag | string    | 索引当前状态，取值如下： <br> "Normal"：正常 <br> "Creating"：正在创建 <br> "Dropping"：正在删除 <br> "Truncating"：正在清空 <br> "Invalid"：无效                                                        |
| Type      | string    | 索引类型，取值如下： <br> "Positive"：正序索引 <br> "Reverse"：逆序索引 <br> "Text"：全文索引                                       |
| NotArray| boolean   | 索引的任意一个字段是否允许为数组，取值如下： <br> "true"：不允许为数组 <br> "false"：允许为数组    |
| dropDups  | boolean   | 暂不开放                                     |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

列举集合 sample.employee 中所有索引的信息

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