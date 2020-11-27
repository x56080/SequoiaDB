##语法##
***db.collectionspace.collection.listIndexes\(\)***

枚举[索引](manual/Distributed_Engine/Architecture/Data_Model/index.md)，执行此方法会将指定集合下的索引信息全部显示出来。

##参数##

无

##字段信息##

| 参数名    | 参数类型  | 描述   | 
| ------    | --------  | ------ |
| name      | string    | 索引名 |
| key       | json 对象 | 索引键，可参考 [indexDef](reference/Sequoiadb_command/SdbCollection/createIdIndex.md)           |
| v         | Int32     | 索引版本号                                   |
| unique    | Boolean   | 索引是否唯一 <br> "true"：该索引为唯一索引，不允许集合中有重复的值 <br> "false"：该索引为普通索引，允许集合中有重复的值                                     | 
| dropDups  | Boolean   | 暂不开放                                     |
| enforced  | Boolean   | 索引是否强制唯一，可参考 [enforced](reference/Sequoiadb_command/SdbCollection/createIdIndex.md)           |
| NotNull   | Boolean   | 索引的任意一个字段是否允许为 null 或者不存在 <br> "true"：不允许为 null 或者不存在 <br> "false"：允许为 null 或不存在    |
| IndexFlag | string    | 索引当前状态 <br> "Normal"：正常 <br> "Creating"：正在创建 <br> "Dropping"：正在删除 <br> "Truncating"：正在清空 <br> "Invalid"：无效                                                        |
| Type      | string    | 索引类型 <br> "Positive"：正序索引 <br> "Reverse"：逆序索引 <br> "Text"：全文索引                                       |

##返回值##

返回游标。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 返回集合 bar 下的所有索引信息

```lang-javascript
> db.foo.bar.listIndexes()
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
    "NotNull": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```
