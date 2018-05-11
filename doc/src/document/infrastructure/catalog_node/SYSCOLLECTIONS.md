##所属集合空间##

SYSCAT

##概念##

SYSCAT.SYSCOLLECTIONS 集合中包含了该集群中所有的用户集合信息。每个用户集合保存为一个文档。

每个文档包含以下字段：

|字段名|类型|描述|
|----|----|----|
|Name|字符串|集合的完整名，为<集合空间>.<集合名>形式。|
|Version|整数|集合的版本号，由1起始，每次对该集合的元数据变更会造成版本号+1。|
|IsMainCL|布尔型|表示集合是否为垂直分区中的主表。|
|MainCLName|字符串|指示集合在垂直分区中的主表。|
|ReplSize|整数|最小复制组，确保任何写操作必须被复制到至少指定数量的节点后返回成功。|
|ShardingKey|对象|分区键，在分区集合中存在。对象包含一个或多个字段，字段名为分区字段名，数值为1或者-1，代表对该列正向或逆向排序。|
|ShardingType|字符串|分区类型，在分区集合中存在。分区类型有：范围分区（Range）和散列分区（Hash）两种。|
|Partition|整数|散列分区的分区大小值，必须为2的幂。|
|CataInfo|数组|集合所在的逻辑节点信息：<br>在单分区集合中，该数组仅包含一个元素，代表该集合所在的分区组。<br>在多分区集合中，该数组中包含一个或多个元素，代表该集合中的每一个取值范围所在的分区组。每个取值范围包括 LowBound 与UpBound，代表其下限与上限，闭合关系为左闭右开。<br>在主表集合中，该数组中包含一个或多个元素，代表该集合中的每一个取值范围所在的子表。每个取值范围包括 LowBound 与UpBound，代表其下限与上限，闭合关系为左闭右开。|
|Attribute|整数|集合的内部属性掩码|
|AttributeDesc|字符串|集合的内部属性掩码描述|
|CompressionType|整数|压缩算法类型掩码|
|CompressionTypeDesc|字符串|压缩算法类型掩码描述|
|EnsureShardingIndex|布尔型|标识集合是否自动使用ShardingKey包含的字段创建名字为"$shard"的索引|

###示例###

一个典型的单分区集合信息如下：

```
{ "Name" : "test.foo", "Version" : 1, "CataInfo" : [ { "GroupID" : 1000 } ] }
```

一个典型的多分区集合信息如下：

```
{
  "Name" : "foo.test",
  "Version" : 1,
  "ShardingKey" : { "Field1" : 1, "Field2" : -1 },
  "ShardingType" : "range" ,
  "ReplSize": 3,
  "Attribute": 0,
  "AttributeDesc": "",
  "CataInfo" :
  [
    {
      "GroupID" : 1000,
      "LowBound" : { "" : MinKey, "" : MaxKey },
      "UpBound" : { "" : MaxKey, "" : MinKey }
    }
  ]
}
```

