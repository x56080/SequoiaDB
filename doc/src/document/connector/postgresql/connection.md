以下操作均在PostgreSQL shell 环境下执行。

##PostgreSQL与SequoiaDB建立关联##

1. 加载SequoiaDB连接驱动

	```lang-javascript
	foo=# create extension sdb_fdw;
	```

2. 配置与SequoiaDB连接参数

	```lang-javascript
	foo=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbUserName', password 'sdbPassword', preferedinstance 'A', transaction 'off');
	```

	>**Note:** 
	>
	> * 如果没有配置数据库密码验证，可以忽略user与password字段。
	> * 如果需要提供多个协调节点地址，options 中的 address 字段可以按格式 'ip1:port1,ip2:port2,ip3:port3'填写。此时，service 字段可填写任意一个非空字符串。
	> * preferedinstance 设置 SequoiaDB 的连接属性。多个属性以逗号分隔，如：preferedinstance '1,2,A'。详细配置请参考 [preferedinstance](reference/Sequoiadb_command/Sdb/setSessionAttr.md) 取值
	> * preferedinstancemode 设置 preferedinstance 的选择模式
	> * sessiontimeout 设置会话超时时间 如：sessiontimeout '100'
	> * transaction 设置 SequoiaDB 是否开启事务，默认为off。开启为on

3. 关联SequoiaDB的集合空间与集合

	```lang-javascript
foo=# create foreign table test (name text, id numeric) server sdb_server options ( collectionspace 'foo', collection 'bar', decimal 'on' ) ;
	```

	>**Note:**
	>
	> * 集合空间与集合必须已经存在于SequoiaDB，否则查询出错。
	> * 如果需要对接SequoiaDB的decimal字段，则需要在options中指定 decimal 'on' 。
	> * pushdownsort 设置是否下压排序条件到 SequoiaDB，默认为on，关闭为off。
	> * pushdownlimit 设置是否下压 limit 和 offset 条件到 SequoiaDB，默认为on，关闭为off。
	> * 开启 pushdownlimit 时，必须同时开启 pushdownsort ，否则可能会造成结果非预期的问题。
	> * 默认情况下，表的字段映射到SequoiaDB中为小写字符，如果强制指定字段为大写字符，创建方式参考“注意事项1”。
	> * 映射 SequoiaDB 的数组类型，创建方式参考“注意事项2”。
	

4. 更新表的统计信息

	```lang-javascript
	foo=# analyze test;
	```

5. 查询

	```lang-javascript
	foo=# select * from test;
	```

6. 写入数据

	```lang-javascript
	foo=# insert into test values('one',3);
	```

7. 更改数据

	```lang-javascript
	foo=# update test set id=9 where name='one';
	```

8. 查看所有的表(show tables;)

	```lang-javascript
	foo=# \d
	```

9. 查看表的描述信息

	```lang-javascript
	foo=# \d test
	```

10. 删除表的映射关系

	```lang-javascript
	foo=# drop foreign table test;
	```

11. 退出PostgreSQL shell环境

	```lang-javascript
	foo=# \q
	```

##使用须知##

1. 数据类型的对应关系

	| SequoiaDB       | PostgreSQL        | 注意事项                                      |
	| --------------- | ----------------- | --------------------------------------------- |
	| int             | smallint          | 当SequoiaDB中的值超过smallint范围时会发生截断 |
	| int             | integer           |                                               |
	| long            | bigint            |                                               |
	| int             | serial            |                                               |
	| long            | bigserial         |                                               |
	| double          | real              | 存在精度问题，SequoiaDB 存储时不是完全一致    |
	| double          | double precision  |                                               |
	| string          | numeric           |                                               |
	| string          | decimal           |                                               |
	| decimal         | decimal           | 需要在创建外表时，指定选项 decimal 为 'on'    |
	| string          | text              |                                               |
	| string          | char              |                                               |
	| string          | varchar           |                                               |
	| binary(type=0)  | bytea             |                                               |
	| date            | date              |                                               |
	| timestamp       | timestamp         |                                               |
	| array           | TYPE[]            | 仅支持一维数组                                |
	| boolean         | boolean           |                                               |
	| null            | text              |                                               |

2. 注意事项

	1. 注意字符的大小写

		SequoiaDB 中的集合空间、集合和字段名均对字母的大小写敏感。

		* 集合空间、集合名大写

		假设SequoiaDB 中存在名为 FOO 的集合空间，BAR 的集合，在PostgreSQL中建立相应的映射表：

		```lang-javascript
		foo=# create foreign table sdb_upcase_cs_cl (name text) server sdb_server options ( collectionspace 'FOO', collection 'BAR' ) ;
		```

		* 字段名大写

		假设SequoiaDB 中存在名为foo的集合空间，bar的集合，而且保存的数据为：

		```lang-diy
		{
		  "_id": {
		    "$oid":"53a2a0e100e75e2c53000006"
		  },
		  "NAME": "test"
		}
		```

		在PostgreSQL中建立相应的映射表：

		```lang-javascript
		foo=# create foreign table sdb_upcase_field ("NAME" text) server sdb_server options ( collectionspace 'foo', collection 'bar' ) ;
		```

		执行查询命令：

		```lang-javascript
		foo=# select * from sdb_upcase_field;
		NAME
		------
		test
		(1 rows)
		```

	2. 映射SequoiaDB中的数据类型

		假设SequoiaDB中存在foo集合空间，bar集合，保存记录为：

		```lang-diy
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

		在 PostgreSQL 中建立相应的映射表：

		```lang-javascript
		foo=# create foreign table bartest (name int[], id int) server sdb_server options ( collectionspace 'foo', collection 'bar' ) ;
		```

		执行查询命令：

		```lang-javascript
		foo=# select * from bartest;
		name    | id
		--------+-----
		{1,2,3} | 123
		```

	3. 连接 SequoiaDB 协调节点错误

		如果 PostgreSQL 连接的 SequoiaDB 协调节点重启，在查询时报错：

		```lang-diy
		ERROR: Unable to get collection "foo.bar", rc = -15
		HINT: Make sure the collectionspace and collection exist on the remote database
		```

		解决方法：

		退出 PostgreSQL shell

		```lang-javascript
		foo=# \q
		```

		重新进入 PostgreSQL shell

		```lang-javascript
		$ bin/psql -p 5432 foo
		```

##调整 PostgreSQL 配置文件##

1. 查看 pg_shell 中默认的配置：

	```lang-javascript
	foo=#\set
	AUTOCOMMIT = 'on'
	PROMPT1 = '%/%R%# '
	PROMPT2 = '%/%R%# '
	PROMPT3 = '>> '
	VERBOSITY = 'default'
	VERSION = 'PostgreSQL 9.3.4 on x86_64-unknown-linux-gnu, compiled by gcc (SUSE Linux) 4.3.4 [gcc-4_3-branch revision 152973], 64-bit'
	DBNAME = 'foo'
	USER = 'sdbadmin'
	PORT = '5432'
	ENCODING = 'UTF8'
	```

2. 调整 pg_shell 查询时，每次获取记录数

	```lang-javascript
	foo=#\set FETCH_COUNT 100
	```

	>Note:
	>
	> * 调整为 pg shell 每次获取100 条记录立即返回记录，然后再继续获取。
	> * 直接在 pg shell 中修改配置，只能在当前 pg_shell 中生效，重新登录 pg_shell 需要重新设置。

3. 修改配置文件，调整 pg shell 查询时，每次获取记录数

	```lang-javascript
	$ /opt/postgresql/bin/pg_config --sysconfdir
	```

	结果为：

	```lang-javascript
	$ /opt/postgresql/etc
	```

	>**Note:**
	>
	> 如果显示目录不存在，自己手动创建即可。

	```lang-javascript
	$ mkdir -p /opt/postgresql/etc
	```

	将需要修改的参数写入配置文件中(需重启psql使配置生效)：

	```lang-javascript
	$ echo "\\set FETCH_COUNT 100" >> /opt/postgresql/etc
	```

4. 调整 pg shell 的日志级别

	编辑 /opt/postgresql/data/postgresql.conf 文件，将

	```lang-javascript
	client_min_messages = notice
	```

	改为：

	```lang-javascript
	client_min_messages = debug1
	```

5. 调整 pg 引擎的日志级别

	编辑 /opt/postgresql/data/postgresql.conf 文件，将

	```lang-javascript
	log_min_messages = warning
	```

	改为：

	```lang-javascript
	log_min_messages = debug1
	```
