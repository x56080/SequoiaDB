##安装##

首先，你需要获取最新版本的[Spark](http://spark.apache.org/downloads.html)。Spark的安装使用请参考[Spark官方文档](http://spark.apache.org/docs/latest)。

然后，从[SequoiaDB](http://www.sequoiadb.com)或者[maven仓库](http://mvnrepository.com/artifact/com.sequoiadb)下载相应版本的Spark-SequoiaDB连接器和SequoiaDB Java驱动。

###Spark-SequoiaDB连接组件的环境要求###

- JDK 1.7+
- Scala 2.11
- Spark 2.0.0+

###Spark-SequoiaDB连接器的安装###

将Spark-SequoiaDB连接组件和SequoiaDB Java驱动的jar包复制到Spark安装路径下的jars目录下即可。 注意需要将jar包复制到每一台机器的 Spark安装路径下。