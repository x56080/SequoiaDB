以下操作均在MySQL shell 环境下执行。
##连接MySQL与SequoiaDB##

1. 登录MySQL shell

 ```lang-bash
 # mysql -u root -p
 ```

2. 加载SequoiaDB插件

 ```lang-sql
 mysql> install plugin sequoiadb soname 'ha_sequoiadb.so';
 ```

3. 查询存储引擎

 ```lang-sql
 mysql> show storage engines;
 ```
当显示出现以下记录时，说明SequoiaDB插件安装成功

 ```lang-sql
 | SequoiaDB | YES | SequoiaDB storage engine | YES | NO | NO |
 ```

4. 创建数据库实例

 ```lang-sql
 mysql> create database cs;
 mysql> use cs;
 ```

5. 创建表

 ```lang-sql
 mysql> create table cl(a int, c text) engine = SequoiaDB comment="{cl_options:{ShardingKey:{a:1,b:-1},ShardingType:\"range\"}}";
 ```

6. 数据操作

 ```lang-sql
 mysql> insert into cl values(1, "SequoiaDB test");
 mysql> select * from cl;
 ```

##配置说明##

1. 配置SequoiaDB连接地址  
   默认的SequoiaDB连接地址为“localhost:11810”，可以通过以下两种方式修改该地址：  
   (1)修改配置文件/etc/my.cnf，在[mysqld]下添加如下配置：  

 ```lang-ini
 sequoiadb_conn_addr=192.168.20.37:11810,192.168.20.38:11810
 ```

	  注意：修改配置文件后需要重新启动MySQL服务

   (2)通过MySQL shell修改  

 ```lang-sql
 mysql> SET GLOBAL sequoiadb_conn_addr='192.168.20.37:11810,192.168.20.38:11810';
 ```

	  注意：通过shell方式进行的配置为临时有效，当重启MySQL服务后配置将失效。如果需要配置永久生效则必须通过修改配置文件。  
 
 配置完成后，可以通过以下命令查看配置结果

 ```lang-sql
 mysql> show variables like 'sequoiadb%';
 ```
 
2. 建表参数  
 在mysql上创建表时，可以通过comment参数传入配置信息，comment参数为json格式，具体配置参数如下表：
 
| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | --- | ------ | ------ |
| cl_options | json | 创建集合的相关参数。详见SequoiaDB集合相关参数，如：分区键(ShardingKey)、分区类型(ShardingType)等均在options中添加。| 否	|

 示例：  
 在SequoiaDB上创建分区键为“{a:1,b:-1}”，分区类型为范围分区的集合cl  

 ```lang-sql
 mysql> create table cl(a int, b int, c text) engine = SequoiaDB comment="{cl_options:{ShardingKey:{a:1,b:-1},ShardingType:\"range\"}}";
 ```
