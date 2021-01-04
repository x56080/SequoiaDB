
SYSCAT.SYSCOLLECTIONS 集合中包含了该集群中所有的用户集合信息，每个用户集合保存为一个文档。

每个文档包含以下字段：

|字段名|类型|描述|
|----|----|----|
|Name|string|集合的完整名，为<集合空间>.<集合名>形式 |
|Version|number|集合的版本号，由 1 起始，每次对该集合的元数据变更会造成版本号 +1 |
|IsMainCL|boolean|表示集合是否为垂直分区中的主表 |
|MainCLName|string|指示集合在垂直分区中的主表 |
|ReplSize|number|最小复制组，确保任何写操作必须被复制到至少指定数量的节点后返回成功 |
|ShardingKey|object|分区键，为<字段名>:<数值>形式，仅在分区集合中存在<br>字段名为分区字段名，数值取值如下：<br> 1：正向排序 <br>-1：逆向排序 |
|ShardingType|string|分区类型，仅在分区集合中存在 <br> 分区类型有范围分区（Range）和散列分区（Hash）两种 |
|Partition|number|散列分区的分区大小值，必须为 2 的幂 |
|CataInfo|array|集合所在的逻辑节点信息 <br>1. 在单分区集合中，该数组仅包含一个元素，代表该集合所在的复制组 <br>2. 在多分区集合中，该数组中包含一个或多个元素，代表该集合中的每一个取值范围所在的复制组；每个取值范围包括 LowBound 与UpBound，代表其下限与上限，闭合关系为左闭右开 <br>3. 在主表集合中，该数组中包含一个或多个元素，代表该集合中的每一个取值范围所在的子表；每个取值范围包括 LowBound 与 UpBound，代表其下限与上限，闭合关系为左闭右开 |
|Attribute|number|集合的内部属性掩码|
|AttributeDesc|string|集合的内部属性掩码描述|
|CompressionType|number|压缩算法类型掩码|
|CompressionTypeDesc|string|压缩算法类型掩码描述|
|EnsureShardingIndex|boolean|标识集合是否自动使用 ShardingKey 包含的字段创建名字为"$shard"的索引|

###示例###

- 一个典型的单分区集合信息如下：

 ```lang-json
 { "Name" : "sample.employee", "Version" : 1, "CataInfo" : [ { "GroupID" : 1000 } ] }
 ```

- 一个典型的多分区集合信息如下：

 ```lang-json
 {
   "Name" : "sample.employee",
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

