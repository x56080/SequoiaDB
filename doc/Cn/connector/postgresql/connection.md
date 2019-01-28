以下操作均在PostgreSQL shell 环境下执行。

##PostgreSQL与SequoiaDB建立关联##

1) 加载SequoiaDB连接驱动

<pre class="prettyprint lang-javascript">
foo=# create extension sdb_fdw;</pre>

2) 配置与SequoiaDB连接参数

<pre class="prettyprint lang-javascript">
foo=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbadmin', password 'mypassword');</pre>

**Note:** 

如果没有配置数据库密码验证，可以忽略user与password字段。

3) 关联SequoiaDB的集合空间与集合

<pre class="prettyprint lang-javascript">
foo=# create foreign table test (name text, id numeric) server sdb_server options ( collectionspace 'chen', collection 'test', decimal 'on' ) ;</pre>

**Note:**

集合空间与集合必须已经存在于SequoiaDB，否则查询出错。

如果需要对接SequoiaDB的decimal字段，则需要在options中指定 decimal 'on' 。

默认情况下，表的字段映射到SequoiaDB中为小写字符，如果强制指定字段为大写字符，创建方式参考注意事项1。

映射SequoiaDB 的数组类型，创建方式参考注意事项2。

4) 更新表的统计信息

<pre class="prettyprint lang-javascript">
foo=# analyze test;</pre>

5) 查询

<pre class="prettyprint lang-javascript">
foo=# select * from test;</pre>

6) 写入数据

<pre class="prettyprint lang-javascript">
foo=# insert into test values('one',3);</pre>

7) 更改数据

<pre class="prettyprint lang-javascript">
foo=# update test set id=9 where name='one';</pre>

8) 查看所有的表(show tables;)

<pre class="prettyprint lang-javascript">
foo=# \d</pre>

9) 查看表的描述信息

<pre class="prettyprint lang-javascript">
foo=# \d test</pre>

10) 删除表的映射关系

<pre class="prettyprint lang-javascript">
foo=# drop foreign table test;</pre>

11) 退出PostgreSQL shell环境

<pre class="prettyprint lang-javascript">
foo=# \q</pre>

##使用须知##

1) 数据类型的对应关系

SequoiaDB         PostgreSQL          注意事项
----------------- ------------------- ---------------------------------------------
int               smallint            当SequoiaDB中的值超过smallint范围时会发生截断
int               integer
long              bigint
int               serial
long              bigserial
double            real                存在精度问题，sdb存储时不是完全一致
double            double precision
string            numeric
string            decimal
string            text
string            char
string            varchar
binary(type=0)    bytea
date              date
timestamp         timestamp
array             TYPE[]              仅支持一维数组
boolean           boolean
null              text

2) 注意事项

2.1) 注意字符的大小写

SequoiaDB 中的集合空间、集合和字段名均对字母的大小写敏感。

2.1.1) 集合空间、集合名大写

假设SequoiaDB 中存在名为TEST的集合空间，CHEN的集合，在PostgreSQL中建立相应的映射表：

<pre class="prettyprint lang-javascript">
foo=# create foreign table sdb_upcase_cs_cl (name text) server sdb_server options ( collectionspace 'TEST', collection 'CHEN' ) ;</pre>

2.1.2) 字段名大写

假设SequoiaDB 中存在名为foo的集合空间，bar的集合，而且保存的数据为：

<pre class="prettyprint lang-diy">
{
  "_id": {
    "$oid":"53a2a0e100e75e2c53000006"
  },
  "NAME": "test"
}</pre>

在PostgreSQL中建立相应的映射表：

<pre class="prettyprint lang-javascript">
foo=# create foreign table sdb_upcase_field ("NAME" text) server sdb_server options ( collectionspace 'foo', collection 'bar' ) ;</pre>

执行查询命令：

<pre class="prettyprint lang-javascript">
foo=# select * from sdb_upcase_field;</pre>

查询结果为：

<pre class="prettyprint lang-javascript">
NAME
------
test
(1 rows)</pre>

2.2) 映射SequoiaDB中的数据类型

假设SequoiaDB中存在foo集合空间，bar集合，保存记录为：

<pre class="prettyprint lang-diy">
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
}</pre>

在 PostgreSQL 中建立相应的映射表：

<pre class="prettyprint lang-javascript">
foo=# create foreign table bartest (name int[], id int) server sdb_server options ( collectionspace 'foo', collection 'bar' ) ;</pre>

执行查询命令：

<pre class="prettyprint lang-javascript">
foo=# select * from bartest;</pre>

查询结果：

<pre class="prettyprint lang-diy">
name    | id
--------+-----
{1,2,3} | 123</pre>

2.3) 连接 SequoiaDB 协调节点错误

如果 PostgreSQL 连接的 SequoiaDB 协调节点重启，在查询时报错：

<pre class="prettyprint lang-diy">
ERROR: Unable to get collection "chen.test", rc = -15
HINT: Make sure the collectionspace and collection exist on the remote database</pre>

解决方法：

退出 PostgreSQL shell

<pre class="prettyprint lang-javascript">
foo=# \q</pre>

重新进入 PostgreSQL shell

<pre class="prettyprint lang-javascript">
bin/psql -p 5432 foo</pre>

##调整 PostgreSQL 配置文件##

1) 查看 pg_shell 中默认的配置：

<pre class="prettyprint lang-javascript">
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
ENCODING = 'UTF8'</pre>

2) 调整 pg_shell 查询时，每次获取记录数

<pre class="prettyprint lang-javascript">
foo=#\set FETCH_COUNT 100</pre>

Note:

调整为每次 ps_shell 每次获取100 条记录立即返回记录，然后再继续获取。

直接在 pg_shell 中修改配置文件，只能在当前 pg_shell 中生效，重新登录 pg_shell 需要重新设置。

3) 修改配置文件，调整 pg_shell 查询时，每次获取记录数

<pre class="prettyprint lang-javascript">
$ ${PG_HOME}/bin/pg_config --sysconfdir</pre>

结果为：

<pre class="prettyprint lang-javascript">
$ /opt/sequoiadb/pgsql/etc</pre>

**Note:**

如果显示目录不存在，自己手动创建即可。

<pre class="prettyprint lang-javascript">
mkdir -p /opt/sequoiadb/pgsql/etc</pre>

将需要修改的参数写入配置文件中(需重启psql使配置生效)：

<pre class="prettyprint lang-javascript">
$ echo "\\set FETCH_COUNT 100" >> /opt/sequoiadb/pgsql/etc/psqlrc</pre>

4) 调整 pg\shell 的日志级别

<pre class="prettyprint lang-javascript">
$ sed -i 's/#client_min_messages = notice/client_min_messages = debug1/g' pg_data/postgresql.conf</pre>

5) 调整 pg 引擎的日志级别

<pre class="prettyprint lang-javascript">
$ sed -i 's/#log_min_messages = warning/log_min_messages = debug1/g' pg_data/postgresql.conf</pre>
