##概念##

分区键定义了每个集合中所包含数据的分区规则。每一个集合对应一个分区键，分区键中可以包含一个或多个字段。分区键可以在[创建集合](reference/Sequoiadb_command/SdbCS/createCL.md)时指定，也可以集合创建之后进[指定分区键](reference/Sequoiadb_command/SdbCollection/alter.md)。

在编目节点中，每个集合都拥有自己的分区范围，分区范围中每个范围段对应一个分区组，标示该集合的某一数据段坐落于该分区组。

>**Note:**
>
>集合的分区键在创建集合时指定，集合创建成功后分区键无法修改。
>
>在[分区集合](basic_operation/sharding/sharding_collection.md)中，记录插入数据库后无法对分区键值进行更新。

##格式##

-   Range 分区键

    Range 分区键的格式类似于索引键，为一个 JSON 对象。JSON 对象中每一个字段对应分区键的字段，数值为1或者-1，代表正向或逆向排序。

	```lang-diy
	{ <字段1>: <1|-1>, [ <字段2>: <1|-1> ...] }
    ```
	
-   Hash 分区键

    Hash 分区的 ShardingKey 组成方式与 Range 分区方式相同（但字段的正向/逆向不起作用）。Partition 代表了 hash 分区的个数。其值必须是2的幂。范围在[ 2^3 , 2^20 ]。此字段为可选字段。默认为2^12 ，代表我们将整个范围平均划分为4096个分区。设计hash分区的目的是让数据分布更灵活，可以根据需要自由设置每个数据分区承担 hash 分区的范围。ShardingType 如果不填则默认为 hash 分区。

	```lang-diy
	{ ShardingKey: { <字段1>: <1|-1>, [<字段2>: <1|-1>, ...] }, 
	{ ShardingType: "hash" }, 
	[ { Partition: <分区数> } ] }
    ```

##示例##

-   一个包含两个字段，分别为正向和逆向排序的 Range 分区键如下：

	```lang-diy
	{ Field1: 1, Field2: -1 }
    ```

-   Hash 分区键

	```lang-diy
	{ { Field1: 1, Field2: -1 }, { ShardingType: "hash" }, { Partition: 2^12 } }
    ```
