##语法##
***db.collectionspace.createCL( \<name\>, [options] )***

在指定集合空间下创建集合（Collection），集合是数据库中存放文档记录的逻辑对象，任何一条文档记录必须属于一个且仅属于一个集合。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ |------ |
| name | string | 集合名，在同一个集合空间中，集合名必须唯一。 | 是 |
| options | Json 对象 | 在创建集合时，可以通过 options 参数设置集合的其他属性，如指定集合的分区键，是否以压缩的形式插入数据等。 | 否 |

##返回值##

返集合对象，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -6			| 参数错误    | 查看参数是否填写正确	|
| -22			| 集合已存在    | 使用列表查看集合是否已存在	|

[错误码](reference/Sequoiadb_error_code.md)

##格式##

createCL() 方法的定义格式包含 name 和 options 两个参数。name 的值为字符串类型，必须有值。options 是设置集合其他属性参数，目前通过 options 可设置集合的属性有：


| 属性名 | 描述 | 格式 |
| ------ | ------ | ------ |
| ShardingKey | 分区键。| ShardingKey:{\<字段1\> : \<1&#124;-1\>,[\<字段2\> : \<1&#124;-1\>, ...]} |
| ShardingType | 分区方式，默认为 hash  分区。| ShardingType:"hash"&#124;"range" |
| Partition | 分区数，hash 分区时填写，代表了 hash 分区的个数。其值必须是2的幂。范围在[2\^3，2\^20]。默认为4096。| Partition: \<分区数\> |
| ReplSize | 副本数，默认情况下，副本写入个数为1。| ReplSize: \<int num\> |
| Compressed | 是否数据压缩。默认为true。| Compressed:true&#124;false |
| CompressionType | 压缩算法类型。默认为 lzw 算法。| CompressionType:"snappy"&#124;"lzw" |
| IsMainCL | 主分区集合。标示是否为主分区集合，默认为否。| IsMainCL:true&#124;false |
| AutoSplit | 是否自动切分，默认为true。| AutoSplit:true&#124;false |
| Group | 指定创建在某个复制组。| Group: \<group name\> |
| AutoIndexId | 集合是否自动使用_id字段创建名字为"$id"的唯一索引，默认为true。| AutoIndexId:true&#124;false |
| EnsureShardingIndex | 集合是否自动使用ShardingKey包含的字段创建名字为"$shard"的索引，默认为true。| EnsureShardingIndex:true&#124;false |

创建集合的格式为：

 ```
{ "name": "<集合名>", [options] } 
 ```

> **Note:**
> 
> * ShardingKey 是一个 JSON 对象，JSON 对象中每一个字段对应分区键的字段，其值为1或者-1，代表正向或逆向排序。需要分区，必须指定 ShardingKey。
> * ReplSize 是 int 类型，设置写入数据节点的个数，默认为1，当 ReplSize 等于0时，表示写入需要等待所有副本都完成才返回；当 ReplSize 等于-1时，副本写入个数根据活跃副本数变化而变化；手动指定副本写入个数时，不能超出当前组内节点个数。
> * Compressed 为 boolean 类型，为 “true” 时，表示集合中的数据压缩存储，“false” 时表示正常存储数据。开启压缩时，还可通过 CompressionType 指定压缩类型，当前可支持的压缩类型有snappy及lzw。不显式指定C ompressionType 时默认使用 snappy 压缩。snappy 速度较快，而lzw压缩效果较好。
> * 当 options 内设置了多个参数时，用逗号（,）隔开。
> * name 的值不能是空串，含点（.）或者美元符号（$），并且长度不能超过127B，否则操作失败。
> * AutoSplit 必须配合散列分区和域使用，且不能与 Group 同时使用。
> * AutoSplit 不能与 Group 同时使用。
> * 如果在集合中没有指定 AutoSplit，则使用所属域中的 AutoSplit 参数。
> * Group 必须存在于集合空间所属的域中（所有复制组均属于 SYSDOMAIN，即如果集合空间没有指定域，则系统内任意复制组均可）。
> * 压缩算法选择策略：snappy压缩算法是以单条记录为单位进行压缩，记录内部的数据重复度直接影响到压缩率。因此，当记录内部数据重复度较高，如每条记录的字段名、字段值相似，使用 snappy 算法可获得良好的压缩性能。如果记录内部数据重复度很低，但记录间具有更高的相似性，如不同记录之间有相同的字段名，相近的字段值等，则使用 lzw 算法更优。

##示例##

* 在集合空间 foo 下创建集合 bar，不指定分区键

 ```lang-javascript
> db.foo.createCL( "bar" )
 ```

* 在集合空间 foo 下创建集合 bar，指定字段 age 为分区键，升序排序，默认开启了数据压缩功能

 ```lang-javascript
> db.foo.createCL( "bar", { ShardingKey: { "age": 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 1 } )
 ```
