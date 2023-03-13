[^_^]:
     SDB_LIST_SCHEMA

Schema 列表可以列出当前数据库中所有 Schema 的信息。

##标识##

SDB_LIST_SCHEMA

##字段信息##

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| Name   | string | Schema 的名称 |
| Version | number | Schema 的版本号（内部使用）|
| CollectionName | string | 与该 Schema 绑定的集合 |
| Columns | object | Schema 中的字段信息，具体字段说明可参考 Columns 说明 |
| StrictMode | boolean | 是否开启严格模式（暂不开放） |

**Columns 说明**
	
字段 Columns 中每个对象包含如下信息：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| Name   | string | 字段名，格式为 `{<Name>: {<字段定义>}}` |
| Name.Type   | string | 该字段的数据类型 |
| Name.ReadDefault | 与字段 Type 的值对应 | 读默认值<br>通过 Information Schema 读取数据，当记录中不存在指定字段时需返回的默认值 |
| Name.WriteDefault | 与字段 Type 的值对应 | 写默认值<br>通过 Information Schema 写入数据，当新记录中存在原始记录未包含的字段时需补充的默认值 |
| Name.Restrict | int32 | 字段约束的掩码，定义字段需要遵从的约束，默认值为 0，表示无字段约束（暂不开放） |
| Name.RestrictDesc | string | 字段约束（内部自动生成），默认为 ""，对应约束掩码 0（暂不开放） |

##示例##

查看 Schema 列表

```lang-javascript
> db.list(SDB_LIST_SCHEMA)
```

输出结果如下：

```lang-json
{
  "Collection": "sample.employee",
  "Columns": {
    "Name1": {
      "Type": "int32",
      "ReadDefault": 5,
      "WriteDefault": 10,
      "Restrict": 0,
      "RestrictDesc": ""
    }
  },
  "Name": "s1",
  "StrictMode": false,
  "Version": 1,
  "_id": {
    "$oid": "63ea1be587c7ecd8f0c332b4"
  }
}
```