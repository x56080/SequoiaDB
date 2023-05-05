[^_^]:
     SYSINDEXES集合


SYSCAT.SYSINDEXES 集合中包含了该集群中所有的索引信息，每个索引保存为一个文档。

每个文档包含以下字段：

| 字段名            | 类型    | 描述                                              |
|-------------------|---------|---------------------------------------------------|
| Collection        | string  | 集合名 |
| CLUniqueID        | number  | 集合的唯一 ID |
| Name              | string  | 索引名 |
| IndexDef          | object  | 索引定义 |
| IndexDef.name     | string  | 索引名 |
| IndexDef.UniqueID | number  | 索引的唯一 ID |
| IndexDef.key      | object  | 索引键 |
| IndexDef.v        | number  | 索引版本号 |
| IndexDef.unique   | boolean | 索引是否唯一，true 表示该索引唯一，不允许集合中有重复的值 |
| IndexDef.dropDups | boolean | 是否自动删除相同的键值（内部使用） |
| IndexDef.enforced | boolean | 索引是否强制唯一，true 表示不可存在一个以上全空的索引键 |
| IndexDef.NotNull  | boolean | 索引的任意一个字段是否允许为 null 或者不存在，true 表示不允许为 null 或者不存在 |
| IndexDef.NotArray | boolean | 索引的任意一个字段是否允许为数组，true 表示不允许为数组 |
| IndexDef.Global   | boolean | 是否为全局索引，true 表示为全局索引 |
| IndexDef.Mappings | object    | 全文索引字段在 Elasticsearch 的映射关系（仅已配置[字段映射][field_mapping]的索引显示） |
| IndexFlag         | string  | 索引当前状态，取值如下： <br> "Normal"：正常 <br> "Creating"：正在创建 <br> "Dropping"：正在删除 <br> "Truncating"：正在清空 <br> "Invalid"：无效 |
| Type              | string  | 索引类型，取值如下： <br> "Positive"：正序索引 <br> "Reverse"：逆序索引 <br> "Text"：全文索引 |
| Standalone        | boolean | 是否为独立索引，true 表示为独立索引|

**示例**

索引信息如下：

```lang-json
{
  "_id": {
    "$oid": "64462a6764b21798e1ae089a"
  },
  "Collection": "sample.employee",
  "CLUniqueID": 382252089345,
  "Name": "idx_1",
  "IndexDef": {
    "name": "idx_1",
    "_id": {
      "$oid": "64462a6764b21798e1ae0898"
    },
    "UniqueID": 382252089345,
    "key": {
      "date_of_birth": "text"
    },
    "v": 0,
    "unique": false,
    "dropDups": false,
    "enforced": false,
    "NotNull": false,
    "NotArray": false,
    "Global": false,
    "Standalone": false,
    "Mappings": {
      "Fields": {
        "date_of_birth": {
          "Type": "date"
        }
      }
    }
  },
  "IndexFlag": "Normal",
  "Type": "Text",
  "ExtDataName": "SYS_382252089345_idx_1"
}
```



[^_^]:
     本文使用的所有引用及链接
[field_mapping]:manual/Distributed_Engine/Operation/Index/text_index.md#索引字段映射
