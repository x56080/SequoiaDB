本文档将介绍 SequoiaDB 巨杉数据库中 MySQL 实例的相关配置。

## 支持的建表选项

| 选项 | 默认值 | 描述 |
| ---- | ------ | ---- |
| AUTO_INCREMENT | 1 | 自增字段的起始值，SequoiaDB 的自增字段不是严格递增，而是趋势递增，可参考 SequoiaDB [自增字段][sequence]章节 |
| CHARACTER SET | utf8mb4 | 字符数据的字符集 |
| COLLATE | utf8mb4_bin | 字符数据的比较规则，不支持忽略大小写的字符比较规则，字符比较对大小写敏感 |
| COMMENT | "" | 表备注信息，用于指定更多 SequoiaDB 引擎的选项，可参考[自定义表配置][config] |
| COMPRESSION | "" | 表压缩类型，选项有 ""（默认压缩类型）、"none"（关闭压缩）、"lzw"、"snappy"，默认压缩类型为"lzw" |
| ENGINE | SEQUOIADB | 表存储引擎，必须指定为 SEQUOIADB 才能使用本分布式存储引擎，一般无需显式指定 |

**示例**

- 分别通过 COMMENT 和 COMPRESSION 创建压缩类型为"snappy"的表（以下两条语句功能完全相同）

   ```lang-sql
   mysql> CREATE TABLE t1 (id INT) ENGINE=SEQUOIADB COMPRESSION='snappy';
   mysql> CREATE TABLE t2 (id INT) COMMENT='sequoiadb:{ table_options: { CompressionType: "snappy" } }';
   ```

- 指定表自增字段起始值为 1000

   ```lang-sql
   mysql> CREATE TABLE tb (id INT AUTO_INCREMENT PRIMARY KEY) AUTO_INCREMENT=1000;
   ```

## 自定义表配置

用户在 MySQL 上创建表时，可以在表选项 COMMENT 中指定关键词"sequoiadb"，并在其后添加一个 json 对象用于传入自定义的表配置参数。格式如下：

```lang-ini
COMMENT [=] "[string,] sequoiadb:{ [table_options:{...}, partition_options:{...}, auto_partition:<true|false>] }"
```

具体配置参数如下表:

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | --- | ------ | ------ |
| string | string |用户自定义注释字符串 | 否 |
| table_options | json | 创建集合的相关参数，详细参数可参考 [SequoiaDB 创建集合选项][createCL]| 否 |
| partition_options | json | 分区的属性，用于指定 RANGE/LIST [分区表][partition]的分区属性，详细参数可参考 [SequoiaDB 创建集合选项][createCL]| 否 |
| auto_partition | boolean | 是否创建分区表，取值为 false 则显式创建非分区表| 否 |

>**Note：**
>
>* use_partition 已弃用，3.2.4 版本更名为 auto_partition。
>* 引擎配置项相应更名为 sequoiadb_auto_partition。

**示例**

- 在 SequoiaDB 上创建根据时间进行范围切分的表

   ```lang-sql
   mysql> CREATE TABLE business_log(ts TIMESTAMP, level INT, content TEXT, PRIMARY KEY(ts))
          ENGINE=sequoiadb
          COMMENT="Sharding table for example, sequoiadb:{ table_options: { ShardingKey: { ts: 1 }, ShardingType: 'range' } }";
   ```

- 在 引擎配置项 sequoiadb_auto_partition 为 ON 时，指定 auto_partition 为 false 显式创建普通表

   ```lang-sql
   mysql> CREATE TABLE employee(id INT PRIMARY KEY, name VARCHAR(128) UNIQUE KEY)
          ENGINE=sequoiadb
          COMMENT='sequoiadb:{ auto_partition: false }';
   ```

- 在 SequoiaDB 上创建压缩类型为"lzw"的表，通过 ALTER TABLE 修改表压缩类型为"snappy"

   ```lang-sql
   mysql> CREATE TABLE employee2(id INT PRIMARY KEY, name VARCHAR(128) UNIQUE KEY)
          ENGINE=sequoiadb
          COMMENT="sequoiadb:{ auto_partition: true, table_options:{CompressionType : 'lzw'} }";

   mysql> ALTER TABLE employee2 COMMENT="alter table of compress type,sequoiadb:{ auto_partition: true,
          table_options:{CompressionType : 'snappy'} }";
   ```
   > **Note:**
   >
   >ALTER TABLE 支持修改表备注（COMMENT）中的自定义注释，以及更改或追加 table_options 中的配置项，不支持修改 auto_partition。

- 为分区指定 hash 切片数 Partition 属性，在表备注中指定 partition_options 等价于在每个分区备注中单独指定 partition_options（以下两个语句效果完全一致）

   ```lang-sql
   CREATE TABLE goods (
       id INT NOT NULL,
       produced_date DATE,
       name VARCHAR(100),
       company VARCHAR(100)
   )
   COMMENT 'sequoiadb:{ partition_options: { Partition: 8192 } }'
   PARTITION BY RANGE COLUMNS (produced_date)
   SUBPARTITION BY KEY (id)
   SUBPARTITIONS 2 (
       PARTITION p0 VALUES LESS THAN ('1990-01-01'),
       PARTITION p1 VALUES LESS THAN ('2000-01-01'),
       PARTITION p2 VALUES LESS THAN ('2010-01-01')
   );

   CREATE TABLE goods (
       id INT NOT NULL,
       produced_date DATE,
       name VARCHAR(100),
       company VARCHAR(100)
   )
   PARTITION BY RANGE COLUMNS (produced_date)
   SUBPARTITION BY KEY (id)
   SUBPARTITIONS 2 (
       PARTITION p0 VALUES LESS THAN ('1990-01-01')
           COMMENT 'sequoiadb:{ "partition_options": { Partition: 8192 } }',
       PARTITION p1 VALUES LESS THAN ('2000-01-01')
           COMMENT 'sequoiadb:{ "partition_options": { Partition: 8192 } }',
       PARTITION p2 VALUES LESS THAN ('2010-01-01')
           COMMENT 'sequoiadb:{ "partition_options": { Partition: 8192 } }'
   );
   ```

## SequoiaDB引擎配置使用说明

###配置 SequoiaDB 连接与鉴权###

**sequoiadb_conn_addr**

该参数可以配置 MySQL 实例所连接的 SequoiaDB 存储集群，可以配置一个或多个协调节点的地址。使用多个时，地址之间要以逗号隔开。如 `sdbserver1:11810,sdbserver2:11810`。在配置多个地址时，每次连接会从地址中随机选择。在 MySQL 会话数很多时，压力会基本平均地分摊给每个协调节点。

+ 类型：string
+ 默认值："localhost:11810"
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_user**

该参数可以配置 SequoiaDB 集群鉴权的用户。SequoiaDB 鉴权支持明文密码和密码文件两种方式，建议采用密码文件的方式建立连接。

+ 类型：string
+ 默认值：""
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_password**

该参数可以配置 SequoiaDB 集群鉴权的明文密码。

+ 类型：string
+ 默认值：""
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_token** 和 **sequoiadb_cipherfile**

这两个参数可以配置 SequoiaDB 集群鉴权的加密口令和密码文件路径。在配置前，需通过 sdbpassword 工具生成密码文件，具体可参考[数据库密码工具][sdbpasswd]章节。

+ 类型：string
+ 默认值：sequoiadb_token：""，sequoiadb_cipherfile："~/sequoiadb/passwd"
+ 作用范围：Global
+ 是否支持在线修改生效：是

> **Note:**
>
>* 以上配置在命令行修改后，均在建立新连接时才生效，不影响旧连接。
>* 两种密码都配置的情况下，优先使用明文密码。

###配置自动分区功能###

**sequoiadb_auto_partition**

该参数可以配置 MySQL 是否使用自动分区功能。自动分区可以普遍提升 SequoiaDB 的性能。启动时，在 MySQL 上创建表将同步在 SequoiaDB 上创建对应的分区表（散列分区，包含所有复制组）。自动分区时，分区键按顺序优先使用主键字段和唯一索引字段。如果两者都没有，则不做分区。

如果开启自动分区后，部分表不希望被分区，可以在[自定义表配置][config]中指定 auto_partition 为 false。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global
+ 是否支持在线修改生效：是

> **Note:**
>
> 自动分区时，主键或唯一索引只在建表时对应分区键，建表后添加、删除主键或唯一索引都不会更改分区键。

###配置默认副本数###

**sequoiadb_replica_size**

该参数可以配置表默认的写操作需同步的副本数，取值范围为[-1,7]。副本数多时，数据一致性强度高，但性能会有所下降；副本数少时，则反之。具体可参考 SequoiaDB 创建集合的 [ReplSize 参数][createCL]。

+ 类型：int32
+ 默认值：1
+ 作用范围：Global
+ 是否支持在线修改生效：是

###配置批量插入###

**sequoiadb_use_bulk_insert**

该参数可以配置是否开启批量插入功能。批量插入可以提升 SequoiaDB 存储引擎的插入性能。在关闭功能时，MySQL 的批量插入在 SequoiaDB 中是逐条的插入；而开启时，SequoiaDB 存储引擎会把 MySQL 的一个批次分解成若干个 sequoiadb_bulk_insert_size 大小的批次进行插入；例如，MySQL 批量插入 1024 条记录，在 sequoiadb_bulk_insert_size 为 100 时，SequoiaDB 存储引擎会进行 10 次记录数为 100 的批量插入，和 1 次记录数为 24 的批量插入。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_bulk_insert_size**

该参数可以配置 SequoiaDB 每次进行批量插入的记录数，取值范围为[1,100000]。在进行插入性能的调优时，可以根据实际适当调整这个值。

+ 类型：int32
+ 默认值：2000
+ 作用范围：Global
+ 是否支持在线修改生效：是

###配置性能优化参数###

**sequoiadb_selector_pushdown_threshold**

该参数可以配置查询字段下压的触发阈值，取值范围为[0,100]。查询字段不下压时，SequoiaDB 集群总是返回完整记录给 MySQL，由 MySQL 过滤有用字段；而在查询字段下压时，SequoiaDB 集群只返回 MySQL 所需字段。在查询字段个数/表总字段个数的百分比小于等于该阈值时查询字段下压，否则不下压。下压查询字段可以节省了网络传输，但同时也会增加 SequoiaDB 工作量，可以根据实际适当调整。

+ 类型：uint32
+ 默认值：30
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_optimizer_options**

该参数可以配置是否开启优化操作。可填选项如下：<br>
"direct_count"：将 count 语句直接下压到 SeuoiaDB 执行。优化前，SELECT COUNT(*) 会请求 SequoiaDB 返回表中的所有记录，由 MySQL 进行计数；优化后，SELECT COUNT(*) 会对接到 SequoiaDB 的 [SdbCollection.count()][count] 方法，由 SequoiaDB 进行计数。<br>
"direct_update"：将 update 语句直接下压到 SeuoiaDB 执行。优化前，MySQL 会先查询匹配记录，然后逐条记录地下发更新请求。优化后，在符合条件的场景下，只需下发一次更新请求，从而减少网络 IO。<br>
"direct_delete"：将 delete 语句直接下压到 SeuoiaDB 执行。原理与 direct_update 相似，可以减少网络IO。<br>
"direct_sort"：将 order by 和 group by 直接下压到 SeuoiaDB 执行。优化前，排序操作在单个 MySQL 实例上完成。优化后，排序操作由 SequoiaDB 完成。得益于 SequoiaDB 多节点并发排序的能力，性能可以得到提升。<br>
"direct_limit"：将 limit 和 offset 直接下压到 SequoiaDB 执行。优化前，SequoiaDB 需返回所有匹配记录，limit 和 offset 操作在 MySQL 实例进行。优化后，在符合条件的场景下，SequoiaDB 只需返回 limit 指定的记录数。该选项与 direct_sort 结合使用，可以极大地提升分页查询效率。
  
+ 类型：set
+ 默认值："direct_count, direct_delete, direct_update, direct_sort, direct_limit"
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

###配置事务功能###

**sequoiadb_use_transaction**

该参数可以配置事务功能。在业务无需事务功能时，可设置为 OFF，从而节省不必要的开销。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global，Session
+ 是否支持在线修改生效：是

**sequoiadb_rollback_on_timeout**

该参数可以配置记录锁超时是否中断并回滚整个事务。设置为开启后，遇到记录锁超时错误后会中断并且回滚整个事务，否则只会回滚最后一条 SQL 语句。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_lock_wait_timeout**

该参数可以配置事务锁等待超时时间。

+ 类型：int32
+ 默认值：60
+ 取值范围：[0,3600]
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_use_rollback_segments**

该参数可以配置事务是否使用回滚段。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global, Session
+ 是否支持在线修改生效：是

###配置统计信息分析###

**sequoiadb_stats_mode**

该参数可以配置分析（ANALYZE TABLE）模式。<br>
取值如下：<br>
1：表示进行抽样分析，生成统计信息 <br>
2：表示进行全量数据分析，生成统计信息 <br>
3：表示生成默认的统计信息 <br>
4：表示加载统计信息到 SequoiaDB 缓存中 <br>
5：表示清除 SequoiaDB 缓存的统计信息

+ 类型：int32
+ 默认值：1
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_sample_num**

该参数可以指定抽样的记录个数，取值范围为[100,10000]，指定 0 表示缺省。该参数不能与 sequoiadb_stats_sample_percent 同时指定。

+ 类型：int32
+ 默认值：200
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_sample_percent**

该参数可以指定抽样的比例，取值范围为[0.0,100.0]，指定 0.0 表示缺省。表记录数和比例的乘积为抽样的记录数。个数会自动调整在 100~10000 之间（小于 100 调整为 100，大于 10000 调整为 10000）。该参数不能与 sequoiadb_stats_sample_num 同时指定。

+ 类型：double
+ 默认值：0.0
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_cache**

该参数可以配置是否加载 SequoiaDB 统计信息到 MySQL 缓存。统计信息缓存可以帮助生成更高效的访问计划，但会有少量的加载开销。关闭时，则使用默认规则生成访问计划，不使用统计信息。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global
+ 是否支持在线修改生效：是

###其它配置###

**sequoiadb_alter_table_overhead_threshold**

该参数可以配置表开销阈值。当表记录数超过这个阈值，需要全表更新的更改操作将被禁止。这个限制是为了防止对大表误进行更改操作，因为大表的更新会花费较多的时间。该阈值对添加 DEFAULT NULL 的列、数据类型扩容等无需更新的轻量操作不生效。如确认要对大表结构进行更改，在线上调阈值后，重新执行更改操作即可。

+ 类型：int64
+ 默认值：10000000
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_execute_only_in_mysql**

该参数可以配置 DQL/DML/DDL 语句只在 MySQL 执行，不会下压到 SequoiaDB 执行。即 DDL 只会变更 MySQL 的表元数据信息，而不会变更 SequoiaDB 相应表元数据；DQL/DML 所有查询和变更都为空操作，不会实际查询和修改 SequoiaDB 相应表的数据。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_debug_log**

该参数可以配置 MySQL 日志是否会打印 SequoiaDB 存储引擎有关 debug 信息。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_error_level**

该参数可以配置错误级别，可选的配置项有 error 和 warning，用于控制连接器的某些错误返回的方式（报错或警告）。当 SQL 语句执行出错时，若该参数配置为 error，则连接器直接返回错误信息给客户端；若该参数配置为 warning，则连接器返回警告信息给客户端。用户可根据 warning 查询详细的错误信息。该参数仅适用于 update ignore 更新分区键失败时的错误信息。

+ 类型：enum
+ 默认值：error
+ 作用范围：Global
+ 是否支持在线修改生效：是

## SequoiaDB引擎配置修改方式

配置参数有以下三种修改方式：

- 通过工具 sdb_mysql_ctl 修改配置

   ```lang-bash
   $ bin/sdb_mysql_ctl chconf myinst --sdb-auto-partition=OFF
   ```

- 通过实例数据目录下的配置文件 `auto.cnf`，在[mysqld]一栏添加/更改对应配置项

   ```lang-ini
   sequoiadb_auto_partition=OFF
   ```

   > **Note:**
   >
   > 修改配置文件后需要重新启动 MySQL 服务

- 通过 MySQL 命令行修改

   ```lang-sql
   mysql> SET GLOBAL sequoiadb_auto_partition=OFF;
   ```

   > **Note:**
   >
   > 通过命令行方式修改的配置为临时有效，当重启 MySQL 服务后配置将失效，若需要配置永久生效则必须通过配置文件的方式修改。


## MySQL常用系统配置

| 参数名                 | 类型   | 动态生效 | 动态范围   | 默认值  | 说明 |
| ---------------------- | ----   | -------- | ---------- | ------- | ---- |
| max_connections        | int32  | Yes | Global          | 1024    | 客户端最大连接数 |
| max_prepared_stmt_count| int32  | Yes | Global          | 128000  | 最大预编译语句数 |
| sql_mode               | set    | Yes | Global,Session | STRICT_TRANS_TABLES,<br>ERROR_FOR_DIVISION_BY_ZERO,<br>NO_AUTO_CREATE_USER,<br>NO_ENGINE_SUBSTITUTION | SQL 模式，取值意义可参考 [MySQL SQL 模式][sql_mode] |
| character_set_server   | string | Yes | Global,Session | utf8mb4 | 默认字符集 |
| collation_server       | string | Yes | Global,Session | utf8mb4_bin | 默认校对集 |
| default_storage_engine | string | Yes | Global,Session | SequoiaDB | 默认存储引擎 |
| lower_case_table_names | int32  | No  | Global          | 0       | 表名大小写策略，取 0 时，大小写敏感；取 1 时，所有表名均以小写存储；取 2 时，表名以原样存储，但以小写进行比较 |

> **Note:**
>
> * 在系统最大文件句柄数不足时，max_connections 可能被自动调整。如果发现修改该配置没有生效，可检查系统 limit 设置和 MySQL 日志。
> * SequoiaDB 不支持大小写敏感的校对集。
> * 在 Linux 平台下，MySQL 默认表名大小写敏感。更改成大小写不敏感后有可能导致匹配不到原表，应谨慎使用。


[^_^]:
    本文使用的所有引用和链接
[sequence]:manual/infrastructure/Data_Model/sequence.md
[config]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Operation/config.md#自定义表配置
[createCL]:manual/reference/Sequoiadb_command/SdbCS/createCL.md
[partition]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Operation/partition.md
[sdbpasswd]:manual/Maintainance/Mgmt_Tools/sdbpasswd.md#引擎配置
[count]:manual/reference/Sequoiadb_command/SdbCollection/count.md
[sql_mode]:https://dev.mysql.com/doc/refman/5.7/en/sql-mode.html