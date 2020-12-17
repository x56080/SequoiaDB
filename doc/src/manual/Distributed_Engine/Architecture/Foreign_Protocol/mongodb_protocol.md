MongoDB 是一款开源的非关系型数据库，也是目前最流行的非关系型数据库之一。 
SequoiaDB 兼容 MongoDB 语法和协议，用户可以使用 MongoDB 的驱动访问 SequoiaDB 数据库，完成对数据的增、删、查、改操作以及其他操作。

**SequoiaDB 所支持的 MongoDB 版本**

- MongoDB 2.x

- MongoDB 3.x

- MongoDB 4.x

## 使用

假定 sequoiadb 安装目录为 /opt/sequoiadb，协调节点端口号为 11810。

- 配置连接器 fap

  修改协调节点配置文件

  ```lang-bash
  $ cd /opt/sequoiadb
  $ vi conf/local/11810/sdb.conf
  ```
  
  添加一行配置
  
  ```
  fap=fapmongo3
  ```

  > **Note:**  
  >
  > * 如兼容 mongodb 2.x 版本，请添加配置“fap=fapmongo2”；如兼容 mongodb 3.x 版本或者 mongodb 4.x 版本请添加配置“fap=fapmongo3”

- 重启协调节点

  ```lang-bash
  $ ./bin/sdbstop -p 11810
  $ ./bin/sdbstart -p 11810
  ```

- 查看 fap 端口

  连接器 fap 的端口号为协调节点端口号+7

  ```lang-bash
 $ netstat -anp | grep 11817
  tcp        0      0 0.0.0.0:11817           0.0.0.0:*               LISTEN      20462/sequoiadb(118
  ```

  这时可以使用 mongodb 驱动连接到 11817 端口上执行命令。

## 兼容命令

| 命令              | fapmongo2是否支持 | fapmongo3是否支持 |
| ----------------  | ----------------- | ----------------- |
| create collection | 是 | 是 |
| drop collection   | 是 | 是 |
| list collections  | 是 | 是 |
| drop database     | 是 | 是 |
| list databases    | 否 | 是 |
| insert            | 是 | 是 |
| remove            | 是 | 是 |
| update            | 是 | 是 |
| find              | 是 | 是 |
| count             | 是 | 是 |
| aggregate         | 是 | 是 |
| distinct          | 否 | 是 |
| create index      | 是 | 是 |
| drop index        | 是 | 是 |
| list indexes      | 是 | 是 |
| create user       | 是 | 是 |
| drop user         | 是 | 是 |
| list user         | 是 | 是 |
| auth              | 是 | 是 |
| logout            | 是 | 是 |

  > **Note:**  
  >
  > * 查询时不建议使用 batchSize，如 db.cl.find().batchSize( 100 )。尤其在 mongo-java-v3.1 及以下、mongo-shell-v3.0 及以下、mongo-nodejs-v2.2.11 及以下版本中，batchSize 可能被当做 limit，导致查询结果非预期。
  >
  > * fapmongo2 支持 MONGODB-CR 鉴权算法，fapmongo3 支持 SCRAM-SHA-1 鉴权算法。
  > 
  > * createUser 命令在 mongodb-3.x 对应的驱动上无法使用，请改用 runCommand 命令，如：db.runCommand( { createUser: "myuser", pwd: "mypwd" } )。
  >
  > * Java 驱动推荐使用 mongo-java-v3.0 及以上版本，否则聚集语句的查询结果可能不准确。
  >
  > * 删除用户：只能删除当前会话鉴权过的用户。
  >
  > * updateOne/deleteOne 只能在单个分区、单个子表上执行。
  >
  > * fapmongo2 只能使用 fapmongo2 创建的用户进行鉴权。如果当前 SequoiaDB 是由 3.4.1 之前的版本升级上来的，升级之前已存在的用户，fapmongo3 无法使用。如果 fapmongo3 要使用鉴权功能，必须先在当前版本的 SequoiaDB 创建一个新用户。
