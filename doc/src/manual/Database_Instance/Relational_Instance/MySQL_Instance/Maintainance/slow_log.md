SequoiaDB 慢查询日志记录了数据库执行过的缓慢的操作。通过慢日志，用户可以对数据库进行性能分析，能有效帮助用户改善数据库的执行情况。

##开启慢查询日志##

慢查询日志功能可以通过修改相关配置参数来启动

下述以实例 myinst（实例端口号为 3306）、实例组件安装目录 `/opt/sequoiasql/mysql` 为例，介绍具体配置步骤。

1. 确保实例已加入 [实例组][instance_group]

2. 编辑实例配置文件，并根据实际情况调整配置参数的取值

    ```lang-bash
    $ vim /opt/sequoiasql/mysql/database/3306/auto.cnf
    ```
    在文件末尾添加以下内容：

    ```lang-ini
    # 开启慢查询日志
    slow_query_log=ON
    # 配置慢查询时间阈值
    long_query_time=0.5
    # 配置慢查询输出形式
    log_output="TABLE"
    ```

    > **Note:**
    >
    > 示例中仅列举部分配置参数，完整的配置参数列表可参考 [配置][config] 章节的**配置 SequoiaDB 慢日志**部分。

3. 重启实例

##查看慢查询日志##

可以查询 mysql.sdb_slow_log 表获取慢查询日志。mysql.sdb_slow_log 表字段定义介绍：

| 字段名                 | 类型          | 说明 |
| --------------------- | ------------- | ---- |
| start_time            | timestamp(6)  | 开始时间 |
| end_time              | timestamp(6)  | 结束时间 |
| query_id              | varchar(28)   | 查询ID, 用于关联协调节点和数据节点的慢查询信息 |
| digest                | varchar(32)   | 查询摘要标识符 |
| digest_text           | longtext      | 查询摘要文本 |
| user_host             | mediumtext    | 用户主机 |
| query_time            | double        | 查询耗时，单位为毫秒 |
| lock_time             | double        | 元数据加锁时间，单位为毫秒 |
| rows_sent             | int           | 发送到客户端的行数 |
| db                    | varchar(512)  | 数据库 |
| last_insert_id        | int           | 上次成功插入生成的自增字段值 |
| insert_id             | int           | 用户插入时指定的自增字段值 |
| instance_id           | int           | MySQL 实例的 ID |
| instance_url          | varchar(255)  | MySQL 实例的主机名和端口号 |
| sql_text              | mediumblob    | SQL 语句内容 |
| thread_id             | bigint        | 线程 ID |
| connect_count         | int           | 调用 sdb::connect 接口次数 |
| connect_time          | double        | 调用 sdb::connect 接口耗时，单位为毫秒 |
| get_cl_count          | int           | 调用 sdbCollectionSpace::getCollection 接口次数 |
| get_cl_time           | double        | 调用 sdbCollectionSpace::getCollection 接口耗时，单位为毫秒 |
| begin_trans_count     | int           | 调用 sdb::transactionBegin 接口次数 |
| begin_trans_time      | double        | 调用 sdb::transactionBegin 接口耗时，单位为毫秒 |
| exec_inner_sql_count  | int           | 调用 sdb::exec 接口次数 |
| exec_inner_sql_time   | double        | 调用 sdb::exec 接口耗时，单位为毫秒 |
| find_count            | int           | 调用 sdbCollection::query 接口次数 |
| find_time             | double        | 调用 sdbCollection::query 接口耗时，单位为毫秒 |
| aggregate_count       | int           | 调用 sdbCollection::aggregate 接口次数 |
| aggregate_time        | double        | 调用 sdbCollection::aggregate 接口耗时，单位为毫秒 |
| get_count_count       | int           | 调用 sdbCollection::getCount 接口次数 |
| get_count_time        | double        | 调用 sdbCollection::getCount 接口耗时，单位为毫秒 |
| insert_count          | int           | 调用 sdbCollection::insert 接口次数 |
| insert_time           | double        | 调用 sdbCollection::insert 接口耗时，单位为毫秒 |
| update_count          | int           | 调用 sdbCollection::update 接口次数 |
| update_time           | double        | 调用 sdbCollection::update 接口耗时，单位为毫秒 |
| delete_count          | int           | 调用 sdbCollection::del 接口次数 |
| delete_time           | double        | 调用 sdbCollection::del 接口耗时，单位为毫秒 |
| get_detail_count      | int           | 调用 sdbCollection::getDetail 接口次数 |
| get_detail_time       | double        | 调用 sdbCollection::getDetail 接口耗时，单位为毫秒 |
| get_index_stat_count  | int           | 调用 sdbCollection::getIndexStat 接口次数 |
| get_index_stat_time   | double        | 调用 sdbCollection::getIndexStat 接口耗时，单位为毫秒 |
| ha_retry_times        | int           | 实例组元数据同步产生的语句重试次数 |
| ha_sync_wait_time     | double        | 实例组元数据同步耗时，单位为毫秒 |
| pushdown_info         | text          | 调用 SequoiaDB C++ 驱动接口时下压的参数 |


如需使用 JSON 实例查询慢日志，可以通过 `HAInstanceGroup_<name>.HASlowLog` 集合查询。其中 name 为实例组组名。例如在 `sql_group` 实例组的实例，可以访问 `HAInstanceGroup_sql_group.HASlowLog` 集合。

> **Note:**
>
> SequoiaDB C++ 驱动接口详情参考[官网 API 手册][cpp_api]

##查看进行中的慢查询状态##

如果慢查询仍在进行中，它不会写入慢日志中，此时可以通过查询 information_schema.processlist_for_sequoiadb 视图或执行 show processlist for sequoiadb 命令获取信息。

1. 查询 information_schema.processlist_for_sequoiadb 视图。视图字段说明：

    | 字段名                 | 类型          | 说明 |
    | --------------------- | ------------- | ---- |
    | ID                    | bigint        | 会话 ID |
    | USER                  | varchar(32)   | 用户名 |
    | HOST                  | varchar(64)   | 主机名 |
    | DB                    | varchar(64)   | 数据库 |
    | COMMAND               | varchar(16)   | 命令类型 |
    | TIME                  | double        | 当前耗时，单位为毫秒 |
    | STATE                 | varchar(64)   | 当前状态 |
    | INFO                  | longtext      | 操作信息 |
    | QUERY_ID              | varchar(28)   | 查询ID, 用于关联协调节点和数据节点的慢查询信息 |
    | CONNECT_COUNT         | int           | 调用 sdb::connect 接口次数 |
    | CONNECT_TIME          | double        | 调用 sdb::connect 接口耗时，单位为毫秒 |
    | GET_CL_COUNT          | int           | 调用 sdbCollectionSpace::getCollection 接口次数 |
    | GET_CL_TIME           | double        | 调用 sdbCollectionSpace::getCollection 接口耗时，单位为毫秒 |
    | BEGIN_TRANS_COUNT     | int           | 调用 sdb::transactionBegin 接口次数 |
    | BEGIN_TRANS_TIME      | double        | 调用 sdb::transactionBegin 接口耗时，单位为毫秒 |
    | EXEC_INNER_SQL_COUNT  | int           | 调用 sdb::exec 接口次数 |
    | EXEC_INNER_SQL_TIME   | double        | 调用 sdb::exec 接口耗时，单位为毫秒 |
    | FIND_COUNT            | int           | 调用 sdbCollection::query 接口次数 |
    | FIND_TIME             | double        | 调用 sdbCollection::query 接口耗时，单位为毫秒 |
    | AGGREGATE_COUNT       | int           | 调用 sdbCollection::aggregate 接口次数 |
    | AGGREGATE_TIME        | double        | 调用 sdbCollection::aggregate 接口耗时，单位为毫秒 |
    | GET_COUNT_COUNT       | int           | 调用 sdbCollection::getCount 接口次数 |
    | GET_COUNT_TIME        | double        | 调用 sdbCollection::getCount 接口耗时，单位为毫秒 |
    | INSERT_COUNT          | int           | 调用 sdbCollection::insert 接口次数 |
    | INSERT_TIME           | double        | 调用 sdbCollection::insert 接口耗时，单位为毫秒 |
    | UPDATE_COUNT          | int           | 调用 sdbCollection::update 接口次数 |
    | UPDATE_TIME           | double        | 调用 sdbCollection::update 接口耗时，单位为毫秒 |
    | DELETE_COUNT          | int           | 调用 sdbCollection::del 接口次数 |
    | DELETE_TIME           | double        | 调用 sdbCollection::del 接口耗时，单位为毫秒 |
    | GET_DETAIL_COUNT      | int           | 调用 sdbCollection::getDetail 接口次数 |
    | GET_DETAIL_TIME       | double        | 调用 sdbCollection::getDetail 接口耗时，单位为毫秒 |
    | GET_INDEX_STAT_COUNT  | int           | 调用 sdbCollection::getIndexStat 接口次数 |
    | GET_INDEX_STAT_TIME   | double        | 调用 sdbCollection::getIndexStat 接口耗时，单位为毫秒 |
    | HA_RETRY_TIMES        | int           | 实例组元数据同步产生的语句重试次数 |
    | HA_SYNC_WAIT_TIME     | double        | 实例组元数据同步耗时，单位为毫秒 |
    | PUSHDOWN_INFO         | longtext      | 调用 SequoiaDB C++ 驱动接口时下压的参数 |

    > **Note:**
    >
    > SequoiaDB C++ 驱动接口详情参考[官网 API 手册][cpp_api]


2. 执行 show processlist for sequoiadb 命令。返回结果字段参考 information_schema.processlist_for_sequoiadb 视图。



[^_^]:
     本文使用的所有引用及链接
[config]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/config.md
[instance_group]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Installation/instance_group.md
[cpp_api]:manual/Database_Instance/Json_Instance/Development/cpp_driver/cpp_api.md
