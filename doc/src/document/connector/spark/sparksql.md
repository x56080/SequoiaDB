##开始使用 SparkSQL##

SparkSQL 是 Spark 下处理结构化数据执行的模块，它提供了名为 DataFrames 的程序抽象工具，同时他还能作为分布式的 SQL 查询引擎。

只要 Spark 的安装配置符合要求，通过 SequoiaDB 使用 SparkSQL 也是很简单的。

假设集合名为“test.data”，协调节点在 serverX 和 serverY 上，以下指令可以在 spark-shell 执行，并创建一个临时表来对应 SequoiaDB 的 Collection（集合）：

```lang-javascript
scala> sqlContext.sql("CREATE TEMPORARY TABLE datatable USING com.sequoiadb.spark OPTIONS ( host 'serverX:11810,serverY:11810', collectionspace 'test', collection 'data')")
```

除了特别定义的表模式，其将会扫描整个表同时根据每条记录的字段信息来构建表的模式。如果集合中的记录非常多，处理速度将会很慢。

另一种构建表的方式是使用 CREATE TABLE 指令来构建表模式：

```lang-javascript
scala> sqlContext.sql("CREATE TEMPORARY TABLE datatable ( c1 string, c2 int, c3 int ) USING com.sequoiadb.spark OPTIONS ( host 'serverX:11810,serverY:11810', collectionspace 'test', collection 'data')")
```

>**Note:**
>
>临时表只在它被创建的那一个 Session 期间有效。

###参数说明###

|名称|说明|实际类型|默认值|是否必填|
|---|---|---|---|---|
|host|SequoiaDB协调节点/独立节点地址，多个地址以","分隔。例如："server1:11810,server2:11810"|string|-|是|
|collectionspace|集合空间名称|string|-|是|
|collection|集合名称（不包含集合空间名称）|string|-|是|
|username|用户名|string|""|否|
|password|用户名对应的密码|string|""|否|

以下 query 查询可被用于获取表中的数据

```lang-javascript
scala> sqlContext.sql("select * from datatable").foreach(println)
```

##在 SparkSQL 使用 JDBC##

此处使用的 Thrift JDBC/ODBC 服务器对应着 Hive 0.12 的 HiveServer2。你可以用直线脚本在 Hive0.12 或者 Spark 测试 JDBC 服务器。

Spark 的镜像需要在选项-Phive,-Phivethriftserver 下配置。否则 sbin/start-thriftserver.sh 将会显示以下的错误信息：

```lang-diy
failed to launch org.apache.spark.sql.hive.thriftserver.HiveThriftServer2:
You need to build Spark with -Phive and -Phive-thriftserver.
```

需要启动 JDBC/ODBC server，请执行以下的 Spark 目录内容：

```lang-javascript
$ ./sbin/start-thriftserver.sh
```

此处的脚本接收所有 bin/spark-submit 的命令行选项，同时还有 --hiveconf 选项来置顶 Hive 属性。你可以执行以下命令来获取所有可用的选项。

```lang-javascript
./sbin/start-thriftserver.sh –help
```

服务器默认的监听端口为 localhost:10000 你可以使用以下任意环境变量来重写它：

```lang-javascript
$ export HIVE_SERVER2_THRIFT_PORT=&lt;listening-port&gt;
$ export HIVE_SERVER2_THRIFT_BIND_HOST=&lt;listening-host&gt;
$ ./sbin/start-thriftserver.sh   --master &lt;master-uri&gt;   
...
```

或是系统属性：

```lang-javascript
$ ./sbin/start-thriftserver.sh  --hiveconf hive.server2.thrift.port=&lt;listening-port&gt;   --hiveconf hive.server2.thrift.bind.host=&lt;listening-host&gt;   --master &lt;master-uri&gt;   
...
```

现在可使用直线脚本测试 Thrift JDBC/ODBC server:

```lang-javascript
$ ./bin/beeline
```

在直线脚本连接 JDBC/ODBC server in beeline :

```lang-javascript
beeline> !connect jdbc:hive2://localhost:10000
```

Beeline 直线脚本会询问用户名和密码。在非安全模式下，简单输入 username 和空白密码即可。在安全模式下，请按照 [beeline documentation](https://cwiki.apache.org/confluence/display/Hive/HiveServer2%20Clients) 下的说明来执行。

Hive 的配置将 hive-site.xml 文件移动到 conf 目录下

你也可以使用 Hive 自带的直线脚本。
