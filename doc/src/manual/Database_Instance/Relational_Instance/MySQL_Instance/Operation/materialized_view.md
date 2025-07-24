[^_^]:
    物化视图

物化视图（Materilized View）是基于视图定义的表，用于存储系统预计算好的查询结果。该功能可以极大提升查询语句的执行效率，当系统执行查询语句时，能够直接从物化视图中获取数据，减少表连接或表聚集等操作造成的资源消耗，从而提升查询性能。

##使用##

当基表数据发生变更时，用户需手动更新对应的物化视图。为避免用户频繁进行更新操作，建议在以下场景使用物化视图：

- 查询语句的条件明确，不存在变量。
- 基表变更操作较少，或周期性进行变更操作。

下述将介绍创建物化视图、更新数据以及查看视图信息的相关操作。

###创建物化视图###

物化视图创建成功后，需保证优化选项为开启状态，使查询语句中的视图重写为物化视图。具体操作步骤如下：

1. 基于表 employee 创建视图 MV1

    ```lang-sql
    mysql> CREATE VIEW MV1 AS SELECT * FROM employee WHERE age > 20;
    ```

2. 创建物化视图 MT1 并开启优化选项

    ```lang-sql
    mysql> CREATE TABLE MT1(KEY(id)) AS (SELECT * FROM MV1) MAINTAINED BY USER ENABLE QUERY OPTIMIZATION;
    ```

3. 查看物化视图信息

    ```lang-sql
    mysql> SHOW CREATE TABLE MT1\G
    ```

    输出结果如下，字段 Create Table 包含"ENABLE QUERY OPTIMIZATION"，表示已开启优化选项：

    ```lang-sql
    *************************** 1. row ***************************
           Table: MT1
    Create Table: CREATE TABLE `mt1`(
      KEY `id` (`id`)
    ) ENGINE=SequoiaDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin AS SELECT * FROM `company`.`mv1` MAINTAINED BY USER ENABLE QUERY OPTIMIZATION
    ```

###更新数据###

用户在变更基表数据前，需关闭对应物化视图的优化选项。当基表变更完成后，用户需根据实际情况以全量或增量的方式更新物化视图。以全量更新为例，具体操作步骤如下：

1. 关闭优化选项

    ```lang-sql
    mysql> ALTER TABLE MT1 DISABLE QUERY OPTIMIZATION;
    ```

2. 变更表 employee 的数据

    ```lang-sql
    mysql> INSERT INTO employee(name, age) VALUES("Tom", 25);
    ```

3. 更新物化视图

    ```lang-sql
    mysql> TRUNCATE TABLE MT1;
    mysql> INSERT INTO MT1 SELECT * FROM MV1;
    ```

5. 开启优化选项

    ```lang-sql
    mysql> ALTER TABLE MT1 ENABLE QUERY OPTIMIZATION;
    ```

###查看视图信息###

用户可通过查看视图信息，获取与该视图关联的物化视图。具体操作步骤如下：

1. 查看视图 MV1 的信息

    ```lang-sql
    mysql> SHOW CREATE VIEW MV1\G
    ```

2. 查看输出结果，字段 materialized query table 表示与该视图关联的物化视图

    ```lang-sql
    *************************** 1. row ***************************
                        View: mv1
                 Create View: CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `MV1` AS select `employee`.`id` AS `id`,`employee`.`name` AS `name`,`employee`.`age` AS `age` from `employee` where (`employee`.`age` > 20)
        character_set_client: utf8
        collation_connection: utf8_general_ci
    materialized query table: `company`.`mt1`
    ```

##语法##

###创建物化视图###

```lang-sql
CREATE TABLE [IF NOT EXISTS] tbl_name
[(key_definition, ...)]
[table_options]
[partition_options]
[AS] SELECT * FROM view_name MAINTAINED BY USER [{ DISABLE | ENABLE } QUERY OPTIMIZATION]

key_definition: {
    {INDEX | KEY} [index_name] (col_name,...)
     ...
  | [CONSTRAINT [symbol]] PRIMARY KEY (col_name,...)
     ...
  | [CONSTRAINT [symbol]] UNIQUE [INDEX | KEY]
    [index_name] (col_name,...)
	 ...
}

table_options:
    table_option [[,] table_option] ...

table_option: {
    AUTO_INCREMENT [=] value
  | COMMENT [=] 'string'
  | COMPRESSION [=] {'ZLIB' | 'LZ4' | 'NONE'}
  | ENGINE [=] engine_name
}

partition_options:
    PARTITION BY
        { [LINEAR] HASH(expr)
        | [LINEAR] KEY [ALGORITHM={1 | 2}] (column_list)
        | RANGE{(expr) | COLUMNS(column_list)}
        | LIST{(expr) | COLUMNS(column_list)} }
    [PARTITIONS num]
    [SUBPARTITION BY
        { [LINEAR] HASH(expr)
        | [LINEAR] KEY [ALGORITHM={1 | 2}] (column_list) }
      [SUBPARTITIONS num]
    ]
    [(partition_definition [, partition_definition] ...)]

partition_definition:
    PARTITION partition_name
        [VALUES
            {LESS THAN {(expr | value_list) | MAXVALUE}
            |
            IN (value_list)}]
        [COMMENT [=] 'string' ]
        [(subpartition_definition [, subpartition_definition] ...)]

subpartition_definition:
    SUBPARTITION logical_name
        [COMMENT [=] 'string' ]
```

###开启/关闭优化选项###

```lang-sql
ALTER TABLE tbl_name
    {ENABLE | DISABLE} QUERY OPTIMIZATION
```

###修改物化视图属性###

```lang-sql
ALTER TABLE tbl_name
    [alter_option [, alter_option] ...]
    [partition_options]

alter_option: {
    table_option
  | ADD {INDEX | KEY} [index_name] (col_name,...)
    ...
  | ADD [CONSTRAINT [symbol]] PRIMARY KEY (col_name,...)
    ...
  | ADD [CONSTRAINT [symbol]] UNIQUE [INDEX | KEY]
        [index_name] (col_name,...)
    ...
  | ALGORITHM [=] {DEFAULT | INPLACE | COPY}
  | DROP {INDEX | KEY} index_name
  | DROP PRIMARY KEY
  | LOCK [=] {DEFAULT | NONE | SHARED | EXCLUSIVE}
  | ORDER BY col_name [, col_name] ...
  | RENAME {INDEX | KEY} old_index_name TO new_index_name
  | RENAME [TO | AS] new_tbl_name
}

table_option:
    (see CREATE TABLE options)

partition_options:
    partition_option [partition_option] ...

partition_option: {
    ADD PARTITION (partition_definition)
  | COALESCE PARTITION number
  | REORGANIZE PARTITION partition_names INTO (partition_definitions)
  | ANALYZE PARTITION {partition_names | ALL}
  | CHECK PARTITION {partition_names | ALL}
  | OPTIMIZE PARTITION {partition_names | ALL}
  | REBUILD PARTITION {partition_names | ALL}
  | REPAIR PARTITION {partition_names | ALL}
  | REMOVE PARTITIONING
}
```