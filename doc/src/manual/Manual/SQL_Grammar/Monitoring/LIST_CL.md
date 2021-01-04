##描述##

集合列表 $LIST_CL 列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标识##

$LIST_CL

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Name   | string | 集合完整名 |
| UniqueID | int64 | 集合的 UniqueID，在集群上全局唯一 |
| Version | int32 | 集合的版本号，由 1 起始，每次对该集合的元数据变更会造成版本号+1。|
| Attribute | int32 | 集合的内部属性掩码 |
| AttributeDesc | string | 集合的内部属性掩码描述 |
| CompressionType | int32 | 压缩算法类型掩码 |
| CompressionTypeDesc | string | 压缩算法类型掩码描述 |
| CataInfo | array | 集合所在的逻辑节点信息 |

##示例##

查看集合列表

```lang-javascript
> db.exec( "select * from $LIST_CL" )
```

输出结果如下：

```lang-json
{
  "_id": {
    "$oid": "5cf4b6a307c2e1754b77c5ef"
  },
  "Name": "sample.employee",
  "UniqueID": 2469606195201,
  "Version": 1,
  "Attribute": 1,
  "AttributeDesc": "Compressed",
  "CompressionType": 1,
  "CompressionTypeDesc": "lzw",
  "CataInfo": [
    {
      "GroupID": 1001,
      "GroupName": "db2"
    }
  ]
}
...
```
