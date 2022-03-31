 [^_^]:
    FlinkSQL 连接器-连接

Flink 集群启动成功后，用户可通过 FlinkSQL 客户端访问 SequoiaDB 巨杉数据库。 

##配置读取并发度##

从 SequoiaDB 读取数据时，用户需在 FlinkSQL 客户端中配置读取并发度，以保证性能。配置读取并发度时，建议取值与 SequoiaDB 集群中复制组的数量成比例。如集群中存在 6 个复制组，建议将读取并发度配置为 6、12 或 24。

1. 切换至 Flink 安装目录，并启动 FlinkSQL 客户端

    ```lang-bash
    $ bin/sql-client.sh
    ```

2. 配置读取并发度为 12

    ```lang-sql
    Flink SQL> SET 'parallelism.default'='12';
    ```

##创建映射表##

Flink 与 SequoiaDB 通过映射表进行数据交互。

1. 切换至 Flink 安装目录，并启动 FlinkSQL 客户端

    ```lang-bash
    $ bin/sql-client.sh
    ```

2. 在 Flink 中创建映射表 employee，映射对象为协调节点 `sdbserver1:11810` 所在集群的集合 sample.employee

   ```lang-sql
   Flink SQL> CREATE TABLE employee (id INT, name STRING, age INT)
   WITH (
    'connector' = 'sequoiadb',
    'hosts' = 'sdbserver1:11810',
    'collectionspace' = 'sample',
    'collection' = 'employee'
    );
    ```


##自定义表配置##

Flink 创建映射表语句的格式如下：
   
```lang-sql
CREATE TABLE [IF NOT EXISTS] [catalog_name.][db_name.]table_name (<column1, column2, ...>)
WITH(
'option1'='value1', 
'option2'='value2'
...
);
```

创建成功的映射表，在被使用之前都不会与 SequoiaDB 建立连接，因此不会检测参数 option 所指定配置的正确性。

通过参数 option 可以配置映射表的属性，支持的配置项如下：

| 参数名                  | 类型    | 描述                                                                                                                                                                                                                                                      | 必填 |
| ----------------------- | ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---- |
| connector               | string  | 连接器类型，取值仅支持"sequoiadb"     | 是   |
| hosts                   | string  | SequoiaDB 集群中所有或部分协调节点地址，格式为"sdbserver1:11810,sdbserver2:11810..." <br> 配置多个地址时，需确保地址所指向的协调节点存在于同一集群中 | 是   |
| collectionspace         | string  | 所映射的集合空间名称                  | 是   |
| collection              | string  | 所映射的集合名称                      | 是   |
| username                | string  | SequoiaDB 用户名，默认值为""          | 否   |
| passwordtype            | string  | 输入用户密码时对应的密码类型，默认值为"cleartext"，取值如下：<br>"cleartext"：参数 password 为明文密码 <br>"file"：参数 password 为密码文件路径      | 否   |
| token                   | string  | 使用 SequoiaDB 密码管理工具保存密码到密码文件时所指定的加密令牌    | 否   |
| password                | string  | 与参数 username 对应的 SequoiaDB 用户密码                          | 否   |
| bulksize                | int32   | 将数据从 Flink 插入 SequoiaDB 时，单次允许插入的记录条数，默认值为 500      | 否   |
| maxbulkfilltime         | int64   | 当插入记录条数不满足参数 bulksize 的取值时，操作等待的超时时间，默认值为 300，单位为秒<br> 等待超时后数据将被写入，建议根据业务能容忍的最大延迟进行配置  | 否   |
| splitmode               | string  | 分片生成模式，默认值为"auto"，取值如下：<br>"auto"：自动选择模式 <br>"sharding"：以分区为单位进行并发读取 <br> "datablock"：以集合为单位进行并发读取 <br> 该参数取值为"auto"时，如果查询使用了索引则自动选择"sharding"模式，未使用索引则选择"datablock"模式        | 否   |
| preferredinstance       | string  | 分区优先选择的节点实例，默认值为"M"<br>取值说明可参考 [setSessionAttr()][setSessionAttr] 的参数 PreferredInstance  | 否   |
| preferredinstancemode   | string  | 当多个实例符合参数 preferredinstance 条件时的选择模式，默认值为"random"<br>取值说明可参考 [setSessionAttr()][setSessionAttr] 的参数 PreferredInstanceMode | 否   |
| preferredinstancestrict | boolean | 参数 preferredinstance 指定的实例 ID 都不符合条件时是否报错，默认值为 false，表示不报错    | 否   |
| ignorenullfield         | boolean | 向表中插入数据时忽略值为 null 的字段，默认值为 false，表示不忽略值为 null 的字段             | 否   |
| pagesize                | int32   | insert into select 创建集合空间时指定数据页大小，默认值为 65536 <br> 如果集合空间已存在则忽略该参数               | 否   |
| domain                  | string  | insert into select 创建集合空间时指定所属域 <br> 如果集合空间已存在则忽略该参数              | 否   |
| shardingkey             | json    | insert into select 创建集合时指定分区键  | 否   |
| shardingtype            | string  | insert into select 创建集合时指定分区类型，默认值为"hash"，取值如下：<br> "range"：范围分区<br>"hash"：散列分区   | 否   |
| replsize                | int32   | insert into select 创建集合时指定副本写入数  | 否   |
| compressiontype         | string  | insert into select 创建集合时指定压缩类型，默认值为"lzw"，取值如下：<br> "none"：关闭压缩 <br> "lzw"：lzw 算法压缩 <br> "snappy"：snappy 算法压缩       | 否   |
| autosplit               | boolean | insert into select 创建集合时指定是否自动切分，默认值为 false，表示不自动切分 <br> 该参数必须配合散列分区和域使用，且不能与 group 同时使用              | 否   |
| group                   | string  | insert into select 创建集合时指定创建在某个复制组<br>所指定的复制组必须存在于集合空间所属的域中                                                         | 否   |
|  parallelism            | int32   | Sink 并发度，默认值为 1，取值应小于当前 Flink 集群的总 Slot 数量 <br> 建议取值为 SequoiaDB 集群中协调节点数量的倍数                           | 否   |  
| transactionon           | boolean | Sink 是否开启事务，默认值为 false，表示不开启事务 <br> 建议取值如下： <br> 1）在批量写入的场景下，建议取值为 false，以提高写入效率  <br>  2）在实时写入的场景下，建议取值为 true，以保证数据一致性  <br> 3）在实时写入但不要求数据保持一致性的场景下，建议取值为 false， 以提高写入效率                                                              | 否   |





[^_^]:
    本文使用的所有引用及链接
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md