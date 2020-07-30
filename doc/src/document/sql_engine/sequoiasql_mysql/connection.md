用户安装好 MySQL 实例组件后，可直接通过 MySQL Shell 使用标准的 SQL 语言访问 SequoiaDB。

##连接MySQL实例与数据库分布式存储引擎

**配置 SequoiaDB 连接地址**

SequoiaDB 巨杉数据库默认的连接地址为 `localhost:11810`，用户可通过命令行或修改配置文件两种方式修改连接地址。以下步骤中的路径均为默认安装路径，用户可根据实际情况修改。

   
   * 通过 sdb_mysql_ctl 指定实例名修改 SequoiaDB 连接地址

   ```lang-bash
   $ /opt/sequoiasql/mysql/bin/sdb_mysql_ctl chconf myinst --sdb-conn-addr=sdbserver1:11810,sdbserver2:11810
   ```
   
   修改过程中需要提供该数据库实例 root 用户的密码，若未设置 root 用户的密码，在提示密码时直接输入回车即可
   
   ```
   Changing configure of instance myinst ...
   Enter password:
   ok
   ```

   * 通过实例配置文件修改 SequoiaDB 连接地址

   ```lang-bash
   $ vi /opt/sequoiasql/mysql/database/3306/auto.cnf
   ```

   修改 sequoiadb_conn_addr 内容如下：

   ```config
   sequoiadb_conn_addr=sdbserver1:11810,sdbserver2:11810
   ```

   >   **Note:**
   >
   > 目前 sdb_mysql_ctl 工具仅支持一些简单配置项的修改，建议用户采用配置文件的方式修改配置，修改方式同上。具体配置参考[引擎配置](sql_engine/sequoiasql_mysql/config.md#引擎配置)章节。

**登录 MySQL Shell** 
 
MySQL 支持基于 UNIX 域套接字文件和 TCP/IP 两种连接方式。UNIX  域套接字文件连接属于进程间通信，连接时只需指定对应的套接字文件，不需要使用网络协议所以传输效率比 TCP/IP 连接方式更高，但仅限于本地连接。TCP/IP 连接属于网络通信，支持本地连接（环回接口）和远程连接，同时可以灵活地配置和授权客户端 IP 的访问权限。
 
   * 通过 UNIX 域套接字文件连接

    ```lang-bash
    $ cd /opt/sequoiasql/mysql
    $ bin/mysql -S database/3306/mysqld.sock -u root
    ```
    
    > **Note:** 
    >  
    > SequoiaSQL-MySQL 实例默认无密码，所以无需输入 -p 选项。

   * 通过 TCP/IP 连接
      
      * 本地连接

      ```lang-bash
      $ cd /opt/sequoiasql/mysql
      $ bin/mysql -h 127.0.0.1 -P 3306 -u root
      ```

      * 远程连接  
         
      MySQL 默认未授予远程连接的访问权限，所以需要在服务端对客户端 IP 进行访问授权。
      
      1、创建 sdbadmin 用户，对所有的 IP 都授权访问权限，且设置授权密码 123456

      ```lang-sql
      mysql> GRANT ALL PRIVILEGES ON *.* TO sdbadmin@'%' IDENTIFIED BY '123456' WITH GRANT OPTION;
      mysql> FLUSH PRIVILEGES;
      ```

      2、假设 mysql 服务器地址为 `sdbserver1:3306`，在客户端可以使用如下方式进行远程连接：

      ```lang-bash
      $ /opt/sequoiasql/mysql/bin/mysql -h sdbserver1 -P 3306 -u sdbadmin
      ```
 
**设置 MySQL Shell 登陆密码**

为本地连接的 root 用户设置密码 123456

```lang-sql
mysql> ALTER USER root@localhost IDENTIFIED BY '123456';    
```

> **Note:** 
>  
> 用户设置密码后，登录 MySQL Shell 需要指定 -p 参数输入密码。

## 基本操作示例

以下列举一些简单的操作示例，具体可参考 MySQL 官网：https://dev.mysql.com/doc/refman/5.7/en/

- **创建数据库实例**

 ```lang-sql
 mysql> CREATE DATABASE company;
 mysql> USE company;
 ```

- **创建表**

 ```lang-sql
 mysql> CREATE TABLE employee(id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(128), age INT);
 mysql> CREATE TABLE manager(employee_id INT, department TEXT, INDEX id_idx(employee_id));
 ```

- **基本数据操作**

 ```lang-sql
 mysql> INSERT INTO employee(name, age) VALUES("Jacky", 36);
 mysql> INSERT INTO employee(name, age) VALUES("Alice", 18);
 mysql> INSERT INTO manager VALUES(1, "Wireless Business");
 mysql> SELECT * FROM employee ORDER BY id ASC LIMIT 1;
 mysql> SELECT * FROM employee, manager WHERE employee.id=manager.employee_id;
 mysql> UPDATE employee SET name="Bob" WHERE id=1;
 mysql> DELETE FROM employee WHERE id=2;
 ```
 
- **创建索引**

 ```lang-sql
 mysql> ALTER TABLE employee ADD INDEX name_idx(name(30));
 ```

- **删除表和数据库实例**

 ```lang-sql
 mysql> DROP TABLE employee, manager;
 mysql> DROP DATABASE company;
 ```