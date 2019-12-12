##MySQL实例组件使用注意事项##


1. 不支持创建外键；

2. 时间戳类型字段取值范围为：1902-01-01 00:00:00.000000 至 2037-12-31 23:59:59.999999；

3. 索引键不超过3072字节。通过MySQL实例创建的索引，不可直接在SequoiaDB上对索引执行删除或修改操作；

4. 复合唯一索引仅支持所有字段null值重复，不允许部分字段null值重复，例如：允许出现(null,null)和(null,null)重复值，但不允许出现(1,null)和(1,null)重复值；

5. 建表时默认创建SequoiaDB分区表。如在分区表建立新的唯一索引，必须包含其分区键（分区键依次优先选择主键和唯一键）。如需更改创建分区表配置见[配置说明](sql_engine/sequoiasql_mysql/config.md#引擎配置)；

6. 不支持在BINARY、VARBINARY、TINYBLOB、BLOB、MEDIUMBLOB、LONGBLOB、JSON类型的字段上创建索引；

7. 一个 MySQL 实例节点仅可与一个SequoiaDB集群对接，不支持跨多个SequoiaDB集群；

8. 默认字符集为utf8mb4，不支持忽略大小写的字符比较规则，字符比较对大小写敏感；

9. VARCHAR、TEXT在SequoiaDB上查询比较时不会忽略尾部空格，而MySQL会忽略尾部空格，因此对于尾部含有空格的字符串，查询结果可能会不准确；

10. DDL操作不支持事务。

11. 支持自增字段，MySQL表自增字段对应SequoiaDB的集合[自增字段](data_model/auto_increment.md)，只保证趋势递增，不保证连续递增，使用时需注意以下事项:
    * auto_increment_offset：该配置项主要解决多活主网下自增字段冲突问题，而SDB作为分布式数据库，自身能保证自增字段全局递增而不冲突，故该配置项不生效；
    * auto_increment_increment：自增字段数值由SDB生成，故除了创建表以外的行为（例如插入数据），该配置项不起作用，如需要修改步长，可以在SDB侧通过[SdbCollection.setAttributes()](reference/Sequoiadb_command/SdbCollection/setAttributes.md) 对对应的自增序列步长属性进行修改；
    * 自增字段类型为无符号长整型时，取值范围为：0-9223372036854775807。
    * 自增字段数值增长到自身类型最大值时，会报“序列值超出范围”错误，而不是继续插入最大值。

