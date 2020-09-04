以下操作均在 PostgreSQL Shell 环境下执行。

##连接PostgreSQL实例组件与存储引擎##

1. 加载 SequoiaDB 连接驱动

   ```lang-sql
   sample=# create extension sdb_fdw;
   ```

2. 配置与 SequoiaDB 连接参数

   ```lang-sql
   sample=# create server sdb_server foreign data wrapper sdb_fdw options( address '127.0.0.1', service '11810', user 'sdbUserName', password 'sdbPassword', preferedinstance 'A', transaction 'off' );
   ```

   >**Note：** 
   >
   > * 如果没有配置数据库密码验证，可以忽略 user 与 password 字段。
   > * 如果需要提供多个协调节点地址，options 中的 address 字段可以按格式 'ip1:port1,ip2:port2,ip3:port3' 填写。此时，service 字段可填写任意一个非空字符串。
   > * preferedinstance：设置 SequoiaDB 的连接属性，多个属性以逗号分隔，如：preferedinstance '1,2,A'。取值可参考 [preferedinstance](reference/Sequoiadb_command/Sdb/setSessionAttr.md)；
   > * preferedinstancemode：设置 preferedinstance 的选择模式，取值可参考 [preferedinstancemode](reference/Sequoiadb_command/Sdb/setSessionAttr.md)；
   > * sessiontimeout：设置会话超时时间 如：sessiontimeout '100'；
   > * transaction：设置 SequoiaDB 是否开启事务，默认为 off，开启为 on；
   > * cipher：设置是否使用密文模式输入密码，默认为 off，开启为 on，关于密文模式的介绍，可参考[密码管理](database_management/security/system_security.md)；
   > * token：设置加密令牌；
   > * cipherfile：设置密文文件路径，默认为 `~/sequoiadb/passwd`。

3. 关联 SequoiaDB 的集合空间与集合

   ```lang-sql
   sample=# create foreign table test (name text, id numeric) server sdb_server options ( collectionspace 'sample', collection 'employee', decimal 'on' );
   ```

   >**Note:**
   >
   > * 所关联的集合空间与集合必须已经存在于 SequoiaDB，否则查询出错。
   > * 如果需要对接 SequoiaDB 的 decimal 字段，则需要在 options 中指定 decimal 'on' 。
   > * pushdownsort：设置是否下压排序条件到 SequoiaDB，默认为 on，关闭为 off；
   > * pushdownlimit：设置是否下压 limit 和 offset 条件到 SequoiaDB，默认为 on，关闭为 off。
   > * 开启 pushdownlimit 时，必须同时开启 pushdownsort，否则可能会造成结果非预期的问题。
   > * 默认情况下，表的字段映射到 SequoiaDB 中为小写字符，如果强制指定字段为大写字符，创建方式参考“注意事项”。
   > * 映射 SequoiaDB 的数组类型，创建方式参考“注意事项”。

4. 更新表的统计信息

   ```lang-sql
   sample=# analyze test;
   ```

5. 查询

   ```lang-sql
   sample=# select * from test;
   ```

6. 写入数据

   ```lang-sql
   sample=# insert into test values( 'one', 3 );
   ```

7. 更改数据

   ```lang-sql
   sample=# update test set id = 9 where name = 'one';
   ```

8. 查看所有的表( show tables; )

   ```lang-sql
   sample=# \d
   ```

9. 查看表的描述信息

   ```lang-sql
   sample=# \d test
   ```

10. 删除表的映射关系

   ```lang-sql
   sample=# drop foreign table test;
   ```

11. 退出 PostgreSQL Shell 环境

   ```lang-sql
   sample=# \q
   ```

##使用须知##

**数据类型的对应关系**

| PostgreSQL	     | API              | 注意事项                                      |
| ----------------- | ---------------- | --------------------------------------------- |
| smallint          | int              | 当 API 中的值超过 smallint 范围时会发生截断   |
| integer           | int              |                                               |
| bigint            | long             |                                               |
| serial            | int              |                                               |
| bigserial         | long             |                                               |
| real              | double           | 存在精度问题，SequoiaDB 存储时不是完全一致    |
| double precision  | double           |                                               |
| numeric           | decimal / string | 在创建外表时，指定选项 decimal 为 'on', numeric 映射对应 decimal ，否则对应 string   |
| decimal           | decimal / string | 在创建外表时，指定选项 decimal 为 'on', decimal 映射对应 decimal ，否则对应 string   |
| text              | string           |                                               |
| char              | string           |                                               |
| varchar           | string           |                                               |
| bytea             | binary(type=0)   |                                               |
| date              | date             |                                               |
| timestamp         | timestamp        |                                               |
| TYPE[]            | array            | 仅支持一维数组                                |
| boolean           | boolean          |                                               |
| text              | null             |                                               |

**注意事项**

* 注意字符的大小写

   SequoiaDB 中的集合空间、集合和字段名均对字母的大小写敏感。

   * 集合空间、集合名大写

      假设 SequoiaDB 中存在集合空间 SAMPLE 和集合 EMPLOYEE，在 PostgreSQL 中建立相应的映射表

      ```lang-sql
      sample=# create foreign table sdb_upcase_cs_cl ( name text ) server sdb_server options ( collectionspace 'SAMPLE', collection 'EMPLOYEE' ) ;
      ```

   * 字段名大写

      假设 SequoiaDB 中存在集合空间 sample 和集合 employee，且保存如下记录：

      ```lang-json
      {
          "_id": 
         {
            "$oid":"53a2a0e100e75e2c53000006"
         },
         "NAME": "test"
      }
      ```

      在 PostgreSQL 中建立相应的映射表

      ```lang-sql
      sample=# create foreign table sdb_upcase_field ( "NAME" text ) server sdb_server options ( collectionspace 'sample', collection 'employee' ) ;
      ```

      执行查询命令

      ```lang-sql
      sample=# select * from sdb_upcase_field;
      ```
      
      输出结果： 
       
      ```lang-text
      NAME
      ------
      test
      (1 rows)
      ```

* 映射 SequoiaDB 中的数据类型

   假设 SequoiaDB 中存在 sample 集合空间，employee 集合，且保存如下记录：

   ```lang-json
   {
      "_id": {
         "$oid":"53a2de926b4715450a000001"
      },
      "name": [
         1,
         2,
         3
      ],
      "id": 123
   }
   ```

   在 PostgreSQL 中建立相应的映射表

   ```lang-sql
   sample=# create foreign table employeetest ( name int[], id int ) server sdb_server options ( collectionspace 'sample', collection 'employee' ) ;
   ```

   执行查询命令

   ```lang-sql
   sample=# select * from employeetest;
   ```

   输出结果：

   ```lang-text
   name    | id
   --------+-----
   {1,2,3} | 123
   ```

* 连接 SequoiaDB 协调节点错误

   如果 PostgreSQL 连接的 SequoiaDB 协调节点重启，导致查询时报错如下信息：

   ```lang-sql
   ERROR: Unable to get collection "sample.employee", rc = -15
   HINT: Make sure the collectionspace and collection exist on the remote database
   ```

   解决方法：

   退出 PostgreSQL Shell

   ```lang-sql
   sample=# \q
   ```

   重新进入 PostgreSQL Shell

   ```lang-bash
   $ bin/psql -p 5432 sample
   ```

##调整PostgreSQL配置##

查看 PostgreSQL Shell 中默认的配置

```lang-ini
sample=#\set
AUTOCOMMIT = 'on'
PROMPT1 = '%/%R%# '
PROMPT2 = '%/%R%# '
PROMPT3 = '>> '
VERBOSITY = 'default'
VERSION = 'PostgreSQL 9.3.4 on x86_64-unknown-linux-gnu, compiled by gcc (SUSE Linux) 4.3.4 [gcc-4_3-branch revision 152973], 64-bit'
DBNAME = 'sample'
USER = 'sdbadmin'
PORT = '5432'
ENCODING = 'UTF8'
```

- 调整 PostgreSQL Shell 查询时，每次获取的记录数

   用户可以通过在 PostgreSQL Shell 或配置文件中修改该配置。

   - 通过 PostgreSQL Shell 修改

     调整为 PostgreSQL Shell 每次获取 100 条记录立即返回记录，然后再继续获取

     ```lang-ini
     sample=#\set FETCH_COUNT 100
     ```

     > **Note:**
     >
     > 直接在 PostgreSQL Shell 中修改配置，只能在当前 PostgreSQL Shell 中生效，重新登录 PostgreSQL Shell 需要重新设置。

   - 通过配置文件修改

       1. 查看 PostgreSQL 配置文件路径

         ```lang-bash
         $ /opt/postgresql/bin/pg_config --sysconfdir
         ```

         输出路径如下：

         ```lang-bash
         /opt/postgresql/etc
         ```

         如果显示目录不存在，则需要使用 root 权限手动创建

         ```lang-bash
         # mkdir -p /opt/postgresql/etc
         ```

       2. 将需要修改的参数写入配置文件中

         ```lang-bash
         $ echo "\\set FETCH_COUNT 100" >> /opt/postgresql/etc
         ```

         > **Note:**
         >
         > 修改配置后需要重启 PostgreSQL 使配置生效

- 调整 PostgreSQL Shell 的日志级别

   1. 编辑 `/opt/postgresql/data/postgresql.conf` 文件

     ```lang-ini
     $ vi /opt/postgresql/data/postgresql.conf
     ```

   2. 将 `client_min_messages = notice` 修改为如下值：

     ```lang-ini
     client_min_messages = debug1
     ```

- 调整 PostgreSQL 引擎的日志级别

   1. 编辑 `/opt/postgresql/data/postgresql.conf` 文件

     ```lang-ini
     $ vi /opt/postgresql/data/postgresql.conf
     ```

   2. 将 `log_min_messages = warning` 修改为如下值：

     ```lang-ini
     log_min_messages = debug1
     ```
