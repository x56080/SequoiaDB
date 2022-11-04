[^_^]:
    FlinkSQL 连接器-使用

本文档主要介绍如何通过 Flink 向 SequoiaDB 巨杉数据库读取、写入及更新数据。


##数据读取##

###语法###

```lang-sql
syntax:
{
    select_statement
  | query UNION [ ALL ] query
  | query EXCEPT query
  | query INTERSECT query
}
[ ORDER BY order_item [, order_item]* ]
[ LIMIT {count | ALL} ]
[ OFFSET start_pos {ROW | ROWS} ]
[ FETCH {FIRST | NEXT} | [count] {ROW | ROWS} only ]

order_item:
    expression [ ASC | DESC ]

select_statment:
    SELECT [ ALL | DISTINCT ] { * | projectItem [, projectItem]* }
    FROM table_expression
    [ WHERE boolean_expression ]
    [ GROUP BY groupItem1 [, groupItem]* ]
    [ HAVING boolean_expression ]

table_expression:
    table_name [, table_name]*
   | table_expression [ LEFT | RIGHT | FULL ] JOIN table_expression join_cond
   | sub_query

join_cond:
   ON boolean_expression
   | USING '(' column [, column ]* ')'

projectItem:
    expression [ [ AS ] columnAlias ]

groupItem:
    expression
   | '(' ')'
   | '(' expression [, expression ]* ')'
   | CUBE '(' expression [, expression ]* ')'
   | ROLLUP '(' expression [, expression ]* ')'
   | GROUPING SETS '(' groupItem [, groupItem ]* ')'
```

###示例###

读取表 employee 的所有数据

```lang-sql
Flink SQL> SELECT * FROM employee;
```

##数据写入##

###语法###

```lang-sql
insert:
   INSERT { INTO | OVERWRITE } [catalog_name.][db_name.]table_name VALUES values_row [, values_row ...]

insert_into_select:
   INSERT { INTO | OVERWRITE } [catalog_name.][db_name.]table_name
   select_statement    

select_statment:
    SELECT [ ALL | DISTINCT ] { * | projectItem [, projectItem]* }
    FROM table_expression
    [ WHERE boolean_expression ]
    [ GROUP BY groupItem1 [, groupItem]* ]
    [ HAVING boolean_expression ] 
```

###示例###

在映射表 employee 中插入如下数据：

```lang-sql
Flink SQL> INSERT INTO employee VALUES (1, 'Jacky', 36);
Flink SQL> INSERT INTO employee VALUES (2, 'Alice', 18);
```


##数据更新##

数据更新操作将根据[表配置][connection]分为"upsert"和"retract"模式。在进行数据更新时，需要注意以下事项：

- 所创建的 Source 和 Sink 表必须定义主键，确保表能按主键进行 Hash Shuffle。
- 对于"retract"模式，目前仅支持 Kafka 作为上游数据源。如果 Kafka 源存在多个分区，需要将参数 sink.retract.partitioned-source 设置为 true，并通过 sink.retract.event-ts-field-name 指定事件时间字段，以保证结果的准确性。
- 指定为"retract"模式时，仅支持使用 SequoiaDB 提供的 retract-ogg table format。retract-ogg 的使用方法与 Flink 1.15 及以上版本提供的 ogg-json 一致，只需要将 format 名称替换为 retract-ogg 即可，具体选项可参考 [Flink 官网][flink]。除 Flink 提供的参数外，SequoiaDB 新增参数 retract-ogg.table.primary-keys，用于指定源表的主键字段，多个字段间采用逗号(,)分隔，具体使用方式可参考下述示例。

下述以 Kafka 作为上游数据源为例，介绍不同模式的数据更新步骤。

###upsert 模式###

1. 创建 Source 表

    ```lang-sql
    Flink SQL> CREATE TABLE kafkaSource 
    > (
    >     id BIGINT,
    >     name STRING,
    >     salary DOUBLE,
    >     PRIMARY KEY (`id`) NOT ENFORCED
    > )
    > WITH 
    > (
    >     'connector'='kafka',
    >     'topic'='testTopic',
    >     'properties.bootstrap.servers'='kafkaServer:9092',
    >     'properties.group.id'='testGroup',
    >     'scan.startup.mode'='earliest-offset',
    >     'format'='retract-ogg',
    >     'retract-ogg.table.primary-keys'='id'
    > );
    ```

2. 创建 Sink 表，通过参数 writemode 指定更新模式为"upsert"

    ```lang-sql
    Flink SQL> CREATE TABLE sdbSink 
    > (
    >     id BIGINT,
    >     name STRING,
    >     salary DOUBLE,
    >     PRIMARY KEY (`id`) NOT ENFORCEED
    > )
    > WITH
    > (
    >     'connector'='sequoiadb',
    >     'hosts'='sdbServer1:11810,sdbServer2:11810,sdbServer3:11810',
    >     'collecionspace'='sample',
    >     'collection'='employee',
    >     'writemode'='upsert',
    >     'parallelism'='4'
    > );
    ```

3. 执行数据更新

    ```lang-sql
    Flink SQL> INSERT INTO sdbSink SELECT * FROM kafkaSource;
    ```

###retract 模式###

**Kafka 单分区场景**

1. 创建 Source 表

    ```lang-sql
    Flink SQL> CREATE TABLE kafkaSource 
    > (
    >    -- Reqiured：元数据字段，获取数据所在 Topic、Partition
    >    -- 列名必须为 $kafka-topic、$kafka-partition
    >    `$kafka-topic` STRING METADATA FROM 'topic' VIRTUAL,
    >    `$kafka-partition` INT METADATA FROM 'partition' VIRTUAL,
    >    -- 业务字段
    >    id BIGINT,
    >    name STRING,
    >    salary DOUBLE,
    >    PRIMARY KEY (`id`) NOT ENFORCED
    > )
    > WITH 
    > (
    >    'connector'='kafka',
    >    'topic'='testTopic',
    >     'properties.bootstrap.servers'='kafkaServer:9092',
    >    'properties.group.id'='testGroup',
    >    'scan.startup.mode'='earliest-offset',
    >    'format'='retract-ogg',
    >    'retract-ogg.table.primary-keys'='id'
    > );
    ```

2. 创建 Sink 表，通过参数 writemode 指定更新模式为"retract"

    ```lang-sql
    Flink SQL> CREATE TABLE sdbSink 
    > (
    >    -- 与 Source 表元数据字段保持一致，用于接收数据的 Topic、Partition 信息
    >    `$kafka-topic` STRING,
    >    `$kafka-partition` INT,
    >    id BIGINT,
    >    name STRING,
    >    salary DOUBLE,
    >    PRIMARY KEY (`id`) NOT ENFORCED
    > )
    > WITH
    > (
    >     'connector'='sequoiadb',
    >     'hosts'='sdbServer1:11810,sdbServer2:11810,sdbServer3:11810',
    >     'collecionspace'='sample',
    >     'collection'='employee',
    >     'writemode'='retract',
    >     'parallelism'='4'
    > );
    ```

3. 执行数据更新

    ```lang-sql
    Flink SQL> INSERT INTO sdbSink SELECT * FROM kafkaSource;
    ```

**Kafka 多分区场景**

1. 创建 Source 表

    ```lang-sql
    Flink SQL> CREATE TABLE kafkaSource 
    > (
    >     -- Reqiured：元数据字段，获取数据对应的 Row Kind（日志操作类型）
    >     -- 列名必须为 $extra-row-kind
    >     `$extra-row-kind` INT METADATA FROM 'value.$extra-row-kind' VIRTUAL,
    >     -- 业务字段
    >     id BIGINT,
    >     name STRING,
    >     salary DOUBLE,
    >     -- 事件时间
    >     update_ts TIMESTAMP(6),
    >     PRIMARY KEY (`id`) NOT ENFORCED
    > )
    > WITH 
    > (
    >     'connector'='kafka',
    >     'topic'='testTopic',
    >     'properties.bootstrap.servers'='kafkaServer:9092',
    >     'properties.group.id'='testGroup',
    >     'scan.startup.mode'='earliest-offset',
    >     'format'='retract-ogg',
    >     'retract-ogg.table.primary-keys'='id'
    > );
    ```

2. 创建 Sink 表，通过参数 writemode 指定更新模式为"retract"，并设置 sink.retract.partitioned-source 和 sink.retract.event-ts-field-name 

    ```lang-sql
    Flink SQL> CREATE TABLE sdbSink 
    > (
    >     -- 与 Source 表元数据字段保持一致，用于接收数据的 Topic、Partition 信息
    >     `$extra-row-kind` INT,
    >     id BIGINT,
    >     name STRING,
    >     salary DOUBLE,
    >     update_ts TIMESTAMP(6),
    >     PRIMARY KEY (`id`) NOT ENFORCED
    > )
    > WITH
    > (
    >     'connector'='sequoiadb',
    >     'hosts'='sdbServer1:11810,sdbServer2:11810,sdbServer3:11810',
    >     'collecionspace'='sample',
    >     'collection'='employee',
    >     'writemode'='retract',
    >     'parallelism'='4',
    >     'sink.retract.partitioned-source'='true',       -- 取值为 true，告知 Sink 上游为多分区 Kafka
    >     'sink.retract.event-ts-field-name'='update_ts', -- 指定事件时间为业务字段中的 update_ts
    >     'sink.retract.state-ttl'='10'                   -- 设置状态存活时间为 10min，在网络情况较差的情况下建议设置更大的存活时间
    > );
    ```

3. 开启 Flink CheckPoint 机制，具体操作步骤可参考[保证精确一次性][connection]

4. 执行数据更新

    ```lang-sql
    Flink SQL> INSERT INTO sdbSink SELECT * FROM kafkaSource;
    ```

##参考##

Flink 支持的 table formats 可参考 [Flink 官网][format]。

[^_^]:
    本文使用的所有引用及链接
[connection]:manual/Database_Instance/Relational_Instance/FlinkSQL_Connector/Operation/connection.md
[flink]:https://nightlies.apache.org/flink/flink-docs-release-1.15/docs/connectors/table/formats/ogg/#format-options
[format]:https://nightlies.apache.org/flink/flink-docs-release-1.15/docs/connectors/table/formats/overview/
[connection]:manual/Database_Instance/Relational_Instance/FlinkSQL_Connector/Operation/connection.md#保证精确一次性