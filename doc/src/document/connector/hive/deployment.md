##SequoiaDB 支持的 Hive 版本列表##

  * Apache Hive 0.12.0
  * Apache Hive 0.11.0
  * Apache Hive 0.10.0
  * CDH-5.0.0-beta-2 Hive 0.12.0

**Note:**

1. hive-sequoiadb-apache.jar 为支持 Apache 版 Hive 的 SequoiaDB-Hive-Connector
2. hive-sequoiadb-cdh-5.0.0-beta-2.jar 为支持 CDH5.0.0-beta-2 版本下 Hive-0.12的SequoiaDB-Hive-Connector

##配置方法##

1. 安装和配置好 Hadoop/Hive 环境，启动 Hadoop 环境；

2. 拷贝 SequoiaDB 安装目录下（默认在 /opt/sequoiadb）的 hive-sequoiadb-apache.jar（或者hive-sequoiadb-cdh-5.0.0-beta-2.jar） 和 java/sequoiadb.jar 两个文件拷贝到 hive/lib 安装目录下；

3. 修改 Hive 安装目录下的 bin/hive-site.xml 文件（如果不存在，可拷贝 $HIVE_HOME/conf/hive-default.xml.template为 hive-site.xml 文件），增加如下属性：

```lang-diy
<property>
  <name>hive.aux.jars.path</name>
  <value>file://<HIVE_home>/lib/hive-sequoiadb-{version}.jar,file://<HIVE_HOME>/lib/sequoiadb.jar</value>
  <description>Sequoiadb store handler jar file</description>
</property>

<property>
  <name> hive.auto.convert.join</name>
  <value>false</value>
</property>
```

>**Note:**
>
>需要把这些 jar 包存放 HDFS 上，地址和 file 协议的地址一样。
