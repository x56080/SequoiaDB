##从SparkSQL实例访问SequoiaDB数据库存储引擎##

SparkSQL 是 Spark 下处理结构化数据执行的模块，它提供了名为 DataFrame 的数据抽象工具，同时他还能作为分布式的 SQL 查询引擎。

只要 Spark 的安装配置符合要求，通过 SparkSQL 实例访问 SequoiaDB 是很简单的。

使用 Spark API 以及 Spark 自带的命令行工具 spark-shell、spark-sql、beeline 均可以通过 SQL 访问 SequoiaDB。

##创建SequoiaDB表或视图##

###建表语句###

在 SparkSQL 中创建 SequoiaDB 表的 SQL 语句如下

```lang-sql
create <[temporary] table| temporary view> <tableName> [(schema)] using com.sequoiadb.spark options (<option>, <option>, ...)
```

说明：  

1. temporary 表示为临时表或视图，只在创建表或视图的会话中有效，会话退出后自动删除；
2. 表名后紧跟的 schema 可不填，连接器会自动生成。自动生成的 schema 字段顺序与集合中记录的顺序不一致，因此如果对 schema 的字段顺序有要求，应该显式定义 schema；
3. option 为参数列表，参数是键和值都为字符串类型的键值对，其中值的前后需要有单引号，多个参数之间用逗号分隔。

###参数说明###

|名称|说明|实际类型|默认值|是否必填|
|---|---|---|---|---|
|host|SequoiaDB 协调节点/独立节点地址，多个地址以","分隔。例如："server1:11810,server2:11810"|string|-|是|
|collectionspace|集合空间名称|string|-|是|
|collection|集合名称（不包含集合空间名称）|string|-|是|
|username|用户名|string|""|否|
|passwordtype|密码类型，取值可以为"cleartext","file"。"cleartext"表示参数password为明文密码；"file"表示参数 password 为密码文件路径|string|"cleartext"|否|
|password|用户名对应的密码|string|""|否|
|samplingratio|schema 采样率，取值(0, 1.0]|double|1.0|否|
|samplingnum|schema 采样数量（每个分区），取值大于 0。|long|1000|否|
|samplingwithid|schema 采样时是否带"_id"字段，取值为"true"或"false"。|boolean|false|否|
|samplingsingle|schema 采样时使用一个分区，取值为"true"或"false"。|boolean|true|否|
|bulksize|向 SequoiaDB 集合插入数据时批插的数据量，取值大于 0。|int|500|否|
|partitionmode|分区模式，取值可以是"single","sharding","datablock","auto"。设为 auto 时根据情况自动选择"sharding"或"datablock"。|string|auto|否|
|partitionblocknum|每个分区的数据块数，在按 datablock 分区时有效。取值大于 0。|int|4|否|
|partitionmaxnum|最大分区数量，在按 datablock 分区时有效。取值大于等于 0，等于 0 时表示不限制分区最大数量。由于 partitionMaxNum 的限制，每个分区的数据块数可能与 partitionBlockNum 不同。|int|1000|否|
|preferredinstance|指定分区优先选择的节点实例。取值可以为"M","S","A"（不区分大小写），以及实例 ID 的组合。 如果指定多个"M", "S", "A"实例，则只有第一个生效。实例 ID 取值为 1-255。如果多个实例 ID 和"M"一起指定，则在有多个实例符合时的会在符合的实例中优先选择主节点；而当没有实例符合时，也会在其它节点中优先选择主节点。如果多个实例 ID 和"S"一起指定，则在有多个实例符合时的会在符合的实例中优先选择备节点；而当没有实例符合时，也会在其它节点中优先选择备节点。"A"表示任意节点。如果没有匹配的实例，将随机选择。|string|"A"|否|
|preferredinstancemode|在preferredinstance有多个实例符合时的选择模式，取值可以是"random","ordered"。"random"表示从候选实例中随机选择，"ordered"表示按候选实例的顺序选择。|string|"random"|否|
|preferredinstancestrict|在 preferredinstance 指定的实例 ID 都不符合时是否报错。|boolean|true|否|
|ignoreduplicatekey|向表中插入数据时忽略主键重复的错误。|boolean|false|否|
|ignorenullfield|向表中插入数据时忽略值为 null 的字段。|boolean|false|否|
|pagesize|create table as select 创建集合空间时指定数据页大小。如果集合空间已存在，则忽略该参数。|int|65536|否|
|domain|create table as select 创建集合空间时指定所属域。如果集合空间已存在，则忽略该参数。|string|-|否|
|shardingkey|create table as select 创建集合时指定分区键。|json|-|否|
|shardingtype|create table as select 创建集合时指定分区类型，取值可以是"hash"和"range"。|string|"hash"|否|
|replsize|create table as select 创建集合时指定副本写入数。 |int|1|否|
|compressiontype|create table as select 创建集合时指定压缩类型，取值可以是"none","lzw"和"snappy"。"none"表示不压缩。|string|"none"|否|
|autosplit|create table as select 创建集合时指定是否自动切分。必须配合散列分区和域使用，且不能与 group 同时使用。|boolean|false|否|
|group|create table as select 创建集合时指定创建在某个复制组。group 必须存在于集合空间所属的域中。|string|-|否|

###示例###

假设集合名为“test.data”，协调节点在 serverX 和 serverY 上，以下指令可以在 spark-sql 执行，并创建一个表来对应 SequoiaDB 的 Collection（集合）：

```lang-sql
spark-sql> create table datatable(c1 string, c2 int, c3 int) using com.sequoiadb.spark options(host 'serverX:11810,serverY:11810', collectionspace 'test', collection 'data');
```

也可以不指定 schema，由连接器自动生成：

```lang-sql
spark-sql> create table datatable using com.sequoiadb.spark options(host 'serverX:11810,serverY:11810', collectionspace 'test', collection 'data');
```

创建表或视图之后就可以在表上执行 SQL 语句。以下 query 查询可被用于统计表中的记录数

```lang-sql
spark-sql> select * from datatable;
```

也可以从 SequoiaDB 的一个表向另一个插入数据：

```lang-sql
spark-sql> insert into table t2 select * from t1;
```

如果两个表的 schema 相同，则不需指定列名，否则需要指定。

##存储类型与SparkSQL实例类型映射##

|存储引擎类型|SparkSQL 实例类型|SQL 实例类型|
|---|---|---|
|int32|IntegerType|int|
|int64|LongType|bigint|
|double|DoubleType|double|
|decimal|DecimalType|decimal|
|string|StringType|string|
|ObjectId|StringType|string|
|boolean|BooleanType|boolean|
|date|DateType|date|
|timestamp|TimestampType|timestamp|
|binary|BinaryType|binary|
|null|NullType|null|
|BSON(嵌套对象)|StructType|struct\<field:type,…>|
|array|ArrayType|array\<type>|

##SequoiaDB存储引擎向SparkSQL实例类型转换的兼容性##

Y 表示兼容，N 表示不兼容

|	|ByteType|ShortType|IntegerType|LongType|FloatType|DoubleType|DecimalType|BooleanType|
|---|---|---|---|---|---|---|---|---|
|int32|Y|Y|Y|Y|Y|Y|Y|Y|
|int64|Y|Y|Y|Y|Y|Y|Y|Y|
|double|Y|Y|Y|Y|Y|Y|Y|Y|
|decimal|Y|Y|Y|Y|Y|Y|Y|Y|
|string|Y|Y|Y|Y|Y|Y|Y|N|
|ObjectId|N|N|N|N|N|N|N|N|
|boolean|Y|Y|Y|Y|Y|Y|Y|Y|
|date|Y|Y|Y|Y|Y|Y|Y|N|
|timestamp|Y|Y|Y|Y|Y|Y|Y|N|
|binary|N|N|N|N|N|N|N|N|
|null|Y|Y|Y|Y|Y|Y|Y|Y|
|BSON|N|N|N|N|N|N|N|N|
|array|N|N|N|N|N|N|N|N|

|	|StringType|DateType|TimestampType|BinaryType|ArrayType|StructType|NullType|MapType|
|---|---|---|---|---|---|---|---|---|
|int32|Y|Y|Y|Y|N|N|Y|N|
|int64|Y|Y|Y|Y|N|N|Y|N|
|double|Y|N|N|Y|N|N|Y|N|
|decimal|Y|Y|Y|Y|N|N|Y|N|
|string|Y|Y|Y|Y|N|N|Y|N|
|ObjectId|Y|N|N|Y|N|N|Y|N|
|boolean|Y|N|N|Y|N|N|Y|N|
|date|Y|Y|Y|Y|N|N|Y|N|
|timestamp|Y|Y|Y|Y|N|N|Y|N|
|binary|Y|N|N|Y|N|N|Y|N|
|null|Y|Y|Y|Y|Y|Y|Y|Y|
|BSON|Y|N|N|N|N|Y|Y|Y|
|array|Y|N|N|N|Y|N|Y|N|

> Note:  
> 1. 不支持 SparkSQL 的 CalendarIntervalType 类型；  
> 2. null 转换为任意类型仍为 null；  
> 3. 不兼容类型转换时变为目标类型的零值；  
> 4. date 和 timestamp 与数值类型转换时取其毫秒值；  
> 5. string 如果是数值的字符串类型，则可转为对应的数值时。否则，转换为null；  
> 6. boolean 值转为数值类型时，true 为 1，false 为 0；  
> 7. 数值类型之间转换可能会溢出或损失精度。

