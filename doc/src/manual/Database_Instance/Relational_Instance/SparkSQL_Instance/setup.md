[^_^]:
    SparkSQL 实例-安装部署

本文档将介绍 Spark 和 Spark-SequoiaDB 连接器的安装

##Spark 安装##

1. 下载 Spark [产品包][download_spark]

2. 安装 Spark 

   > **Note:**
   > 
   > 可参考 [Spark 官方文档][install_spark]

3. 从 [SequoiaDB 官网][sequoiadb] 或者 [maven 仓库][spark_sdb_maven]下载相应版本的 Spark-SequoiaDB连接器 和 SequoiaDB Java 驱动

   > **Note:**
   >
   > Spark-SequoiaDB 连接组件的环境要求：
   > - JDK 1.7+
   > - Scala 2.11+
   > - Spark 2.0.0+


##Spark-SequoiaDB 连接器安装##

安装 Spark-SequoiaDB 连接器只需要将 Spark-SequoiaDB 连接组件和 SequoiaDB Java 驱动的 jar 包复制到 Spark 安装路径下的 `jars` 目录下即可。

> **Note:**
>
> 用户需要将 jar 包复制到每一台机器的 Spark 安装路径下。

[^_^]:
    本文使用到的所有链接及引用
[download_spark]:http://spark.apache.org/downloads.html
[install_spark]:http://spark.apache.org/docs/latest
[spark_sdb_maven]:https://mvnrepository.com/artifact/com.sequoiadb
[sequoiadb]:https://download.sequoiadb.com/cn/driver