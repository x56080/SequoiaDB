
用户安装好 PostgreSQL 实例组件后，可直接通过 PostgreSQL Shell 使用标准的 SQL 语言访问 SequoiaDB 巨杉数据库。

连接PostgreSQL实例组件与存储引擎
----

1. 创建 SequoiaSQL PostgreSQL 的 database

   ```lang-bash
   $ bin/sdb_pg_ctl createdb sample myinst
   ```

2. 进入 SequoiaSQL PostgreSQL shell 环境
 
   - 本地连接

     ```lang-bash
     $ bin/psql -p 5432 sample
     ```

   - 远程连接

     假设 PostgreSQL 服务器地址为 `sdbserver1:5432`，在客户端可以使用如下方式进行远程连接：

     ```lang-bash
     $ bin/psql -h sdbserver1 -p 5432 sample
     ```

     >**Note:**
     >
     > - PostgreSQL 默认未授予远程连接的访问权限，所以需要在服务端对客户端 IP 进行访问授权。
     > - 需确保本地创建的数据库与服务器创建的数据库同名，否则将会连接失败。

3. 加载 SequoiaDB 连接驱动

	```lang-sql
	sample=# create extension sdb_fdw;
	```

4. 配置 SequoiaDB 连接参数

    ```lang-sql
	sample=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbUserName', password 'sdbPassword', preferedinstance 'A', transaction 'off');
    ```

    >**Note:**
    >
    > 详细参数说明可参考[关联 SequoiaDB 连接参数说明][connectpara]。

5. 关联 SequoiaDB 集合空间与集合

    ```lang-sql
    sample=# create foreign table test (name text, id numeric) server sdb_server options ( collectionspace 'sample', collection 'employee', decimal 'on' ) ;
    ```

    >**Note:**
    >
    > - 在 PostgreSQL 中建立相应的映射表关联 SequoiaDB 集合时，需要确保映射表的字段名与集合的字段名大小写一致，且映射表的字段类型与集合的字段类型一致；否则，将查询不到相关数据。
    > - 详细参数说明可参考[关联 SequoiaDB 集合空间与集合参数说明][collectionpara]。

6. 更新表的统计信息

	```lang-sql
	sample=# analyze test;
	```

7. 查询

	```lang-sql
	sample=# select * from test;
	```

8. 写入数据

	```lang-sql
	sample=# insert into test values('one',3);
	```

9. 更改数据

	```lang-sql
	sample=# update test set id=9 where name='one';
	```

10. 查看所有的表(show tables;)

	```lang-sql
	sample=# \d
	```

11. 查看表的描述信息

	```lang-sql
	sample=# \d test
	```

12. 删除表的映射关系

	```lang-sql
	sample=# drop foreign table test;
	```

13. 退出 PostgreSQL Shell 环境

	```lang-sql
	sample=# \q
	```

## PostgreSQL与SequoiaDB数据类型映射关系

| PostgreSQL	    |     SequoiaDB    | 注意事项                                      |
| ----------------- | ---------------- | --------------------------------------------- |
| smallint	        | int32            | 当类型为 int32 的值长度超过 smallint 的长度范围时，会发生截断 |
| integer        	| int32              |                                               |
| bigint        	| int64             |                                               |
| serial           	| int32              |                                               |
| bigserial      	| int64             |                                               |
| real              | double           | 存在精度问题，SequoiaDB 存储时不是完全一致    |
| double precision  | double           |                                               |
| numeric           | decimal/string | 在创建外表时，指定选项 decimal 为'on', numeric 映射对应 decimal ，否则对应 string   |
| decimal           | decimal/string | 在创建外表时，指定选项 decimal 为'on', decimal 映射对应 decimal ，否则对应 string   |
| text              | string           |                                               |
| char              | string           |                                               |
| varchar           | string           |                                               |
| bytea             | binary(type=0)   |                                               |
| date              | date             |                                               |
| timestamp         | timestamp        |                                               |
| TYPE[]            | array            | 仅支持一维数组                                |
| boolean           | boolean          |                                               |

## 关联SequoiaDB连接参数说明

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| user   | string   | 数据库用户名 | 否 |
| password | string | 数据库密码 | 否 |
| address | string | 协调节点地址，需要填写多个协调节点地址时，格式为：'ip1:port1,ip2:port2,ip3:port3'，service 字段可填写任意一个非空字符串 | 是 |
| service | string | 协调节点 serviceName | 是 |
| preferedinstance | string | 设置 SequoiaDB 的连接属性，多个属性以逗号分隔，如：preferedinstance '1,2,A'，详细配置可参考 [preferedinstance][preferedinstance] 取值 | 否 |
| preferedinstancemode | string | 设置 SequoiaDB 的连接属性 preferedinstance 的选择模式 | 否 |
| sessiontimeout | string | 设置 SequoiaDB 的连接属性会话超时时间，如：sessiontimeout '100' | 否 |
| transaction | string | 设置 SequoiaDB 与 PostgreSQL 之间的操作是否使用事务，默认为'off'，开启为'on' | 否 | 
| cipher | string | 设置是否使用加密文件输入密码，默认为'off'，开启为'on'<br> 密文模式的介绍可参考[密码管理][system_security] | 否 |
| token | string | 设置加密令牌 | 否 |
| cipherfile | string | 设置加密文件，默认为 `~/sequoiadb/passwd` | 否 |

>**Note:** 
>
> 如果用户没有配置数据库密码验证，可以忽略 user 与 password 字段。


## 关联SequoiaDB集合空间与集合参数说明

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| collectionspace | string | SequoiaDB 中已存在的集合空间 | 是 |
| collection | string | SequoiaDB 中已存在的集合 | 是 |
| decimal | string | 是否对接 SequoiaDB 的 decimal 字段，默认为'off' | 否 |
| pushdownsort | string | 是否下压排序条件到 SequoiaDB，默认为'on'，关闭为'off' | 否 |
| pushdownlimit | string | 是否下压 limit 和 offset 条件到 SequoiaDB，默认为'on'。开启 pushdownlimit 时，必须同时开启 pushdownsort ，否则可能会造成结果非预期的问题 | 否 |

>**Note:**
>
> 用户所指定的集合空间与集合必须已经存在于 SequoiaDB，否则查询出错。


## 调整PostgreSQL配置文件

1. 查看 PostgreSQL Shell 中默认的配置

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

2. 调整 PostgreSQL Shell 查询时每次获取记录数

   ```lang-sql
   sample=#\set FETCH_COUNT 100
   ```

   >**Note:**
   >
   > 调整配置后，每次获取记录数达到 100 时立即返回记录，然后再继续获取。

   用户直接在 PostgreSQL Shell 中修改配置，只能在当前 PostgreSQL Shell 中生效。如果希望配置永久生效，则需要通过配置文件修改相关配置。修改步骤如下：

   - 获取配置文件路径

     ```lang-bash
     $ /opt/postgresql/bin/pg_config --sysconfdir
     ```

     输出结果如下：

     ```lang-bash
     $ /opt/postgresql/etc
     ```

     如果显示目录不存在，则需要手动创建

     ```lang-bash
     $ mkdir -p /opt/postgresql/etc
     ```

   - 将需要修改的参数写入配置文件中

     ```lang-bash
     $ echo "\\set FETCH_COUNT 100" >> /opt/postgresql/etc
     ```

     > **Note:**
     >
     > 用户修改配置后需要重启 psql 使配置生效。

3. 编辑 `/opt/postgresql/data/postgresql.conf` 文件，将如下 PostgreSQL Shell 的日志级别：

   ```lang-ini
   client_min_messages = notice
   ```

   改为：

   ```lang-ini
   client_min_messages = debug1
   ```

4. 编辑 `/opt/postgresql/data/postgresql.conf` 文件，将如下 pg 引擎的日志级别：

   ```lang-ini
   log_min_messages = warning
   ```

   改为：

   ```lang-ini
   log_min_messages = debug1
   ```


常见问题处理
----

如果 PostgreSQL 连接的 SequoiaDB 协调节点重启，在查询时报错

```lang-sql
ERROR: Unable to get collection "sample.employee", rc = -15
HINT: Make sure the collectionspace and collection exist on the remote database
```

解决方法：

重新进入 PostgreSQL Shell

 ```lang-bash
sample=# \q
$ bin/psql -p 5432 sample
 ```


[^_^]:

    本文使用到的所有连接及引用。

[preferedinstance]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[system_security]:manual/Distributed_Engine/Maintainance/Security/system_security.md
[connectpara]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/connection.md#关联SequoiaDB连接参数说明
[collectionpara]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/connection.md#关联SequoiaDB集合空间与集合参数说明