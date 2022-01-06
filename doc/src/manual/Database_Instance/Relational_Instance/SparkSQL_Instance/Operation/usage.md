[^_^]:
    SparkSQL 实例-使用


本文档将介绍 Spark-SequoiaDB 的使用。

##Spark-SequoiaDB 使用##

下述以通过 SparkSQL 创建 SequoiaDB 表为例，创建语句如下：

```lang-sql
create <[temporary] table| temporary view> <tableName> [(schema)] using com.sequoiadb.spark options (<option>, <option>, ...)
```

- temporary：临时表或视图，只在创建表或视图的会话中有效，会话退出后自动删除。

- schema：可不填，连接器会自动生成。自动生成的 schema 字段顺序与集合中记录的顺序不一致，因此如果对 schema 的字段顺序有要求，应该显式定义 schema 。

- option：参数列表，参数是键和值都为字符串类型的键值对，其中值的前后需要有单引号，多个参数之间用逗号分隔。

**option 参数说明**

| 名称     | 类型      | 默认值  | 描述 | 是否必填|
| ---------| --------- | -------- |-------|--------|
|host|string|-|SequoiaDB 协调节点/独立节点地址，多个地址以","分隔，例如："server1:11810,server2:11810"|是|
|collectionspace|string|-|集合空间名称|是|
|collection|string|-|集合名称（不包含集合空间名称）|是|
|username|string|""|用户名|否|
|passwordtype|string|"cleartext"|密码类型，取值如下：<br>"cleartext"：表示参数 password 为明文密码<br>"file"：表示参数 password 为密码文件路径|否|
|password|string|""|用户名对应的密码|否|
|samplingratio|double|1|schema 采样率，取值范围为(0, 1.0]|否|
|samplingnum|int64|1000|schema 采样数量（每个分区），取值大于 0|否|
|samplingwithid|boolean|FALSE|schema 采样时是否带 _id 字段，取值为 true 或 false  |否|
|samplingsingle|boolean|TRUE|schema 采样时使用一个分区，取值为 true 或 false |否|
|bulksize|int32|500|向 SequoiaDB 集合插入数据时批插的数据量，取值大于 0 |否|
|partitionmode|string|auto|分区模式，取值可以是"single"、"sharding"、"datablock"、"auto"；设为"auto"时根据情况自动选择"sharding"或"datablock" |否|
|partitionblocknum|int32|4|每个分区的数据块数，在按 datablock 分区时有效，取值大于 0 |否|
|partitionmaxnum|int32|1000|最大分区数量，在按 datablock 分区时有效，取值大于等于 0，等于 0 时表示不限制分区最大数量<br>由于 partitionMaxNum 的限制，每个分区的数据块数可能与 partitionBlockNum 不同 |否|
|preferredinstance|string|"A"|指定分区优先选择的节点实例，取值可参考 [preferredinstance][parameter]|否|
|preferredinstancemode|string|"random"|在 preferredinstance 有多个实例符合时的选择模式，取值可参考 [preferredinstancemode][parameter]|否|
|preferredinstancestrict|boolean|TRUE|在 preferredinstance 指定的实例 ID 都不符合时是否报错 |否|
|ignoreduplicatekey|boolean|FALSE|向表中插入数据时忽略主键重复的错误 |否|
|ignorenullfield|boolean|FALSE|向表中插入数据时忽略值为 null 的字段 |否|
|configpath|string|-|配置文件路径<br>如果同时在 options 和配置文件中指定同一参数，将优先使用 options 参数进行配置| 否|
|pagesize|int32|65536|create table as select 创建集合空间时指定数据页大小，如果集合空间已存在则忽略该参数 |否|
|domain|string|-|create table as select 创建集合空间时指定所属域，如果集合空间已存在则忽略该参数 |否|
|shardingkey|json|-|create table as select 创建集合时指定分区键 |否|
|shardingtype|string|"hash"|create table as select 创建集合时指定分区类型，取值可以是"hash"和"range" |否|
|replsize|int32|1|create table as select 创建集合时指定副本写入数 |否|
|compressiontype|string|"none"|create table as select 创建集合时指定压缩类型，取值可以是"none"、"lzw"和"snappy"，"none"表示不压缩 |否|
|autosplit|boolean|FALSE|create table as select 创建集合时指定是否自动切分，必须配合散列分区和域使用，且不能与 group 同时使用 |否|
|group|string|-|create table as select 创建集合时指定创建在某个复制组，group 必须存在于集合空间所属的域中 |否|
|ensureshardingindex|boolean|TRUE|create table as select 创建集合时指定是否使用 ShardingKey 包含的字段创建名字为"$shard"的索引 |否|
|autoindexid|boolean|TRUE|create table as select 创建集合时指定是否自动使用字段 _id 创建名字为"$id"的唯一索引 |否|
|strictdatamode|boolean|FALSE|create table as select 创建集合时指定对该集合的操作是否开启严格数据模式 <br>开启严格数据模式后对数值操作将存在以下限制：<br>1）运算过程不改数据类型<br>2）数值运算出现溢出时直接报错|否|
|autoincrement|json|-|create table as select 创建集合时指定集合使用的自增字段<br>自增字段相关说明可参考 [autoincrement][autoincrement] |否|


##示例##

1. 假设集合名为 test.data ，协调节点在 sdbserver1 和 sdbserver2 上，通过 spark-sql 创建一个表来对应 SequoiaDB 的集合

   ```lang-sql
   spark-sql> create table datatable(c1 string, c2 int, c3 int) using com.sequoiadb.spark options(host 'sdbserver1:11810,sdbserver2:11810', collectionspace 'test', collection 'data');
   ```

2. 从 SequoiaDB 的表 t1 向表 t2 插入数据

   ```lang-sql
   spark-sql> insert into table t2 select * from t1;
   ```



[^_^]:
     本文使用的所有引用和链接
[parameter]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[autoincrement]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md