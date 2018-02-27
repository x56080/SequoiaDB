##安装 PostgreSQL##

PostgreSQL 版本： postgresql-9.3.4.tar.gz  
PostgreSQL 安装目录： /opt/postgresql   
PostgreSQL 数据存放目录： /opt/postgresql/data  
PostgreSQL 运行用户：sdbadmin:sdbadmin_group

注：下面示例中，"#"开头的命令为切换到 root 用户运行的命令；"$"开头的命令，为切换到 sdbadmin 用户运行的命令。

1. 下载 PostgreSQL

 下载链接：[PostgreSQL 9.3.4](https://www.postgresql.org/ftp/source/v9.3.4/)

2. 创建安装目录

 ```lang-javascript
 # mkdir -p /opt/postgresql
 # chown -R sdbadmin:sdbadmin_group /opt/postgresql
 ```

3. 解压安装包

 ```lang-javascript
 # cp postgresql-9.3.4.tar.gz /opt
 # cd /opt
 # tar -zxvf postgresql-9.3.4.tar.gz
 ```

4. 源码编译配置

 ```lang-javascript
 # su - sdbadmin
 $ cd /opt/postgresql-9.3.4/
 $ ./configure --prefix=/opt/postgresql
 ```

5. 源码编译

 ```lang-javascript
 $ make
 ```
当编译出现“All of PostgreSQL successfully made. Ready to install.”，说明编译成功。
	> **Note:**
	>
	> PostgreSQL 安装需要依赖 readline。若编译过程提示缺少 readline 库，建议先安装此库。如果选择忽略 readline 编译，则生成的 psql 不支持“上”、“下”、“左”、“右”光标键。
	>
	> 部分系统 readline 安装方法可参考：
	>
	> Ubuntu
	>
	>    ```lang-javascript
	>    # apt-get install libreadline6-dev
	>    ```
	> RedHat
	>
	>    ```lang-javascript
	>    # yum install zlib-devel readline-devel
	>    ```

6. 源码安装 

 ```lang-javascript
 $ make install
 ```
当出现“PostgreSQL installation complete.”，说明 PostgreSQL 相关的可执行文件、头文件和库文件已经成功安装于预定的目录。

7. 设置环境变量
	
	编辑$HOME/.bash_profile，将如下变量添加至 sdbadmin 用户的环境变量中：

	> export PGSQL_HOME=/opt/postgresql  
	> export PATH=$PGSQL_HOME/bin:$PATH  
	> export LD_LIBRARY_PATH=$PGSQL_HOME/lib:$LD_LIBRARY_PATH
	
	注意：不同的 Linux 操作系统需要编辑的启动文件存在差异。大多数 Linux 操作系统只用以下3个启动文件中的一个：
	* $HOME/.bash_profile
	* $HOME/.bash_login
	* $HOME/.profile

8. 使环境变量生效

 ```lang-javascript
 $ source ~/.bash_profile
 ```

9. 创建 PostgreSQL 的数据目录

 ```lang-javascript
 $ mkdir /opt/postgresql/data
 ```

10. 初始化数据目录(该操作只能操作一次)

 ```lang-javascript
 $ /opt/postgresql/bin/initdb -D /opt/postgresql/data
 ```

##安装 SequoiaDB-PostgreSQL 插件##

1. 创建 PostgreSQL 的 lib 目录

 获取 PostgreSQL 的 libdir 路径

 ```lang-javascript
 $ cd /opt/postgresql
 $ PGLIBDIR=$(bin/pg_config --libdir)
 ```

 如果显示的 libdir 目录不存在，则创建该目录：

 ```lang-javascript
 $ mkdir -p ${PGLIBDIR}
 ```

2. 创建PostgreSQL的extension目录

 获取 PostgreSQL 的 sharedir 路径

 ```lang-javascript
 $ PGSHAREDIR=$(bin/pg_config --sharedir)
 ```

 检查 sharedir 目录下是否存在 extension 目录，若不存在，则创建该目录：

 ```lang-javascript
 $ mkdir -p ${PGSHAREDIR}/extension
 ```

3. 安装 PostgreSQL 的扩展文件

 从 SequoiaDB 安装后的 postgresql 目录（默认为/opt/sequoiadb/postgresql）中拷贝 sdb_fdw.so 文件到 PostgreSQL 的 lib 目录，并添加软链接。

 sdb_fdw.so 文件名如 sdb_fdw.so_2.2_23000 ，2.2 代表对应的 SequoiaDB 版本，23000 代表 Release 号。
   
 ```lang-javascript
 $ cp -f /opt/sequoiadb/postgresql/sdb_fdw.so_2.2_23000 ${PGLIBDIR}
 $ cd ${PGLIBDIR}
 $ ln -s sdb_fdw.so_2.2_23000 sdb_fdw.so
 ```

4. 将 sdb_fdw.control 和 sdb_fdw--1.0.sql 脚本拷贝到 extension 目录中：

 ```lang-javascript
 $ cp -f /opt/sequoiadb/postgresql/sdb_fdw.control ${PGSHAREDIR}/extension/
 $ cp -f /opt/sequoiadb/postgresql/sdb_fdw--1.0.sql ${PGSHAREDIR}/extension/
 ```

 sdb_fdw.control 脚本内容：

 ```
 # sdb_fdw extension
 comment = 'foreign data wrapper for SequoiaDB access'
 default_version = '1.0'
 module_pathname = '$libdir/sdb_fdw'
 relocatable = true
 ```
 
 sdb_fdw--1.0.sql 脚本内容：

 ```
 /* contrib/sdb_fdw/sdb_fdw--1.0.sql */
 -- complain if script is sourced in psql, rather than via CREATE EXTENSION
 \echo Use "CREATE EXTENSION sdb_fdw" to load this file. \quit
 CREATE FUNCTION sdb_fdw_handler()
 RETURNS fdw_handler
 AS 'MODULE_PATHNAME'
 LANGUAGE C STRICT;
 CREATE FUNCTION sdb_fdw_validator(text[], oid)
 RETURNS void
 AS 'MODULE_PATHNAME'
 LANGUAGE C STRICT;
 CREATE FOREIGN DATA WRAPPER sdb_fdw
 HANDLER sdb_fdw_handler
 VALIDATOR sdb_fdw_validator;
 ```

##部署 PostgreSQL##

1. 修改 PostgreSQL 的日志配置，日志中增加打印时间信息、连接信息等

 ```lang-javascript
 $ cd /opt/postgresql
 $ vi data/postgresql.conf
 ```

 打印连接信息

 ```
 log_connections = on
 ```

 打印断连信息

 ```
 log_disconnections = on 
 ```

 日志中打印时间，进程id，客户端地址信息

 ```
 log_line_prefix = '%m %p %r'
 ```

 出现错误时，断开当前连接

 ```
 exit_on_error = on
 ```

2. 检查端口是否被占用

 PostgreSQL 默认启动端口为”5432”,检查端口是否被占用(检查操作建议使用 root 用户操作，只有检查端口需要 root 权限，其余操作还是需要在 sdbadmin 用户下操作)

 ```lang-javascript
 # netstat -nap | grep 5432
 ```

 如果5432端口被占用或者希望修改 PostgreSQL 的启动端口，可继续修改/opt/postgresql/data/postgresql.conf文件。如把启动端口改为11700。

 ```lang-javascript
	port = 11700
 ```

3. 启动 PostgreSQL 服务进程（需要使用 sdbadmin 用户执行以下命令）

 ```lang-javascript
 $ bin/postgres -D data/ >> logfile 2>&1 &
  ```

4. 检查 PostgreSQL 是否启动成功

 ```lang-javascript
 # netstat -nap | grep 5432
 tcp   0   0 127.0.0.1:5432     0.0.0.0:*         LISTEN     20502/postgres
 unix  2   [ ACC ]   STREAM    LISTENING   40776754 20502/postgres     /tmp/.s.PGSQL.5432
 ```

5. 创建 PostgreSQL 的 database

 ```lang-javascript
 $ bin/createdb -p 5432 foo
 ```

6. 运行 PostgreSQL shell 命令行客户端

 ```lang-javascript
 $ bin/psql -p 5432 foo
 ```
