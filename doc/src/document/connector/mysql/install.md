##安装 MySQL##

MySQL 版本： mysql-5.7.18.tar.gz  
MySQL 安装目录： /opt/mysql   
MySQL 数据存放目录： /opt/mysql/data  

1. 环境准备 
 gcc版本4.4以上  
 下载并安装boost：[boost 1.59.0](https://nchc.dl.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.gz)  
 下载并安装cmake：[cmake 3.8.1](https://cmake.org/files/v3.8/cmake-3.8.1-Linux-x86_64.tar.gz)   
 下载mysql 5.7.18源码：[mysql 5.7.18](https://cdn.mysql.com//Downloads/MySQL-5.7/mysql-5.7.18.tar.gz)


2. 编译安装MySQL

 ```lang-javascript
 # tar -xzvf boost_1_59_0.tar.gz
 # tar -xzvf mysql-5.7.18.tar.gz
 # cd mysql-5.7.18
 # cmake . -DWITH_BOOST=../thirdparty/boost/boost_1_59_0/ -DCMAKE_INSTALL_PREFIX=/opt/mysql -DMYSQL_DATADIR=/opt/mysql/data -DCMAKE_BUILD_TYPE=Release
 # make -j 4
 # make install
 ```

3. 添加启动服务

 ```lang-javascript
 # cp /opt/mysql/support-files/mysql.server /etc/init.d/mysqld
 # chkconfig --add mysqld
 ```

4. 初始化数据库

 ```lang-javascript
 # /opt/mysql/bin/mysqld --basedir=/opt/mysql/ --datadir=/opt/mysql/data/ --initialize-insecure
 # chown -R mysql:mysql /opt/mysql/
 ```

5. 安装SequoiaDB插件

 ```lang-javascript
 # cp ha_sequoiadb.so /opt/mysql/lib/plugin/
 ```

6. 启动MySQL

 ```lang-javascript
 # service mysqld start
 ```
