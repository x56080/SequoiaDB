##概念##

- SQL 实例是系统提供SQL访问能力的逻辑节点，可以直接配置 MySQL，PostgreSQL和SparkSQL 实例，实现不同 SQL 访问方式。
- SQL 实例将接收的外部请求进行SQL解析，生成内部的执行计划，将执行计划下发至协调节点，并汇总协调节点的应答进行外部响应。
- SQL 实例支持水平伸缩，实例互相独立，一次外部请求只能在一个 SQL 实例内完成。因此，可以根据外部应用的压力来规划SQL 实例的规模。
- SQL 实例需要进行一定的配置，才可以对接至指定的数据库存储引擎。

##操作##

- 创建SQL节点

   指定实例名为myinst，该实例名映射相应的数据目录和日志路径，用户可以根据自己需要指定不同的实例名。

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D pg_data/
   ```

   若端口号被占用，用户可以使用-p参数指定实例端口号：

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D pg_data/ -p 5433
   ```

- 启动SQL节点

   ```lang-bash
   $ bin/sdb_pg_ctl start myinst
   Starting instance myinst ...
   ok (PID: 20502)
   ```

- 查看SQL节点

    ```lang-bash
   $ bin/sdb_pg_ctl status
   INSTANCE   PID      SVCNAME   PGDATA                               PGLOG                                   
   myinst     20502    5432      /opt/sequoiasql/postgresql/pg_data   /opt/sequoiasql/postgresql/pg_data/myinst.log     
   Total: 1; Run: 1
   ```

- 配置对接DB引擎

  系统默认数据库名为 postgres，用户也可以创建指定的数据库，命令如下：

   ```lang-bash
   $bin/sdb_pg_ctl createdb foo myinst
   Creating database myinst ...
   ok
   ```

   连接至数据库，如果没有创建指定的数据库，则连接默认数据库即可：

   ```lang-bash
   $bin/psql -p 5432 foo
   ```

   接下来进行相关的配置操作，请参考 [SQL引擎连接配置](connector/postgresql/connection.md)


- 停止SQL节点

   ```lang-bash
   $ bin/sdb_pg_ctl stop myinst
   Stoping instance myinst (PID: 20502) ...
   ok
   ```

- 删除SQL节点

   ```lang-bash
   $ bin/sdb_pg_ctl delinst myinst
   Deleting instance myinst ...
   ok
   ```

