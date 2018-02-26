##安装 SequoiaSQL##

###安装前准备###

- 使用 root 用户权限来安装 SequoiaSQL
- 检查 SequoiaSQL 产品软件包是否与 SequoiaDB 版本一致 
- 如果需要图形界面模式安装，请确保 X Server 服务正在运行

###安装步骤###

**说明：**

（1）产品包名字以 sequoiasql-oltp-2.8.1-x86_64-enterprise-installer.run 为例；

（2）步骤以命令行方式进行介绍，图形界面按照图像向导提示完成。

- 运行安装程序  
    
  ```lang-javascript
  $ ./sequoiasql-oltp-2.8.1-x86_64-enterprise-installer.run --mode text
  ```

  >**Note:**   
  >执行安装包不添加参数--mode，则进入图形界面。  
  >存在旧版本时，--installmode参数值为cover时，则进入覆盖安装，为upgrade时走升级安装

- 程序提示选择向导语言，输入2，选择中文

  ```
  Language Selection
  Please select the installation language
  [1] English - English
  [2] Simplified Chinese - 简体中文
  Please choose an option [1] :2
  ```

- 输入安装路径后按回车（默认安装在 /opt/sequoiasqloltp ）

  ```
  ----------------------------------------------------------------------------
  由BitRock InstallBuilder评估本所建立
  
  欢迎来到 SequoiaSQL Server 安装程序

  ------------------------------------------------------------
  请指定 SequoiaSQLServer 将会被安装到的目录
  安装目录 [/opt/sequoiasqloltp]:
  ```

- 提示输入用户名和用户组（默认创建 sdbadmin 用户和 sdbadmin_group 用户组），该用户名用于运行 SequoiaSQL 服务

  ```
  ------------------------------------------------------------
  数据库管理用户配置
  配置用于启动 SequoiaSQL 的用户名、用户组和密码
  用户名 [sdbadmin]:
  用户组 [sdbadmin_group]:
  ```

- 提示输入该用户的密码和确认密码（默认密码为 sdbadmin ）

  ```
  密码 [********]:
  确认密码 [********]:
  ```

- 系统提示开始安装，需要用户确认

  ```
  ------------------------------------------------------------
  设定现在已经准备将 SequoiaSQL Server 安装到您的电脑.
  您确定要继续? [Y/n]: 
  ```
    
- 安装完成

  ```
  正在安装 SequoiaSQL Server 于您的电脑中，请稍候。
  安装中
  0% ______________ 50% ______________ 100%
  ########################################
  添加了系统服务: Ssql-oltp.
  #
  ------------------------------------------------------------
  安装程序已经完成安装 SequoiaSQL Server 于你的电脑中.
  ```

##部署 SequoiaSQL##

1. 切换用户和目录

   ```lang-javascript
   $ su - sdbadmin
   $ cd /opt/sequoiasqloltp
   ```

2. 创建实例

   指定实例名为myinst，该实例名映射相应的数据目录和日志路径，用户可以根据自己需要指定不同的实例名。

   ```lang-javascript
   $ bin/sdb_sql_ctl addinst myinst -D pg_data/
   ```

   查看实例：

   ```lang-javascript
   $ bin/sdb_sql_ctl listinst
   NAME       PGDATA                         PGLOG                                   
   myinst     /opt/sequoiasqloltp/pg_data    /opt/sequoiasqloltp/myinst.log     
   Total: 1
   ```

2. 检查端口是否被占用

   SequoiaSQL 默认启动端口为5432,检查端口是否被占用。(检查操作建议使用 root 用户操作，只有检查端口需要 root 权限，其余操作还是需要在 sdbadmin 用户下操作)

   ```lang-javascript
   $ netstat -nap | grep 5432
   ```

   如果5432端口被占用或者希望修改 SequoiaSQL 的启动端口，则执行：

   ```lang-javascript
   $ bin/sdb_sql_ctl chconf myinst -p 5433
   Changing configure of instance myinst ...
   Parameter port requires a server restart to take effect
   ok
   ```

3. 启动实例进程

   ```lang-javascript
   $ bin/sdb_sql_ctl start myinst
   Starting instance myinst ...
   ok (PID: 20502)
   ```
    
    查看实例状态
    
    ```lang-javascript
   $ bin/sdb_sql_ctl status
   INSTANCE   PID      SVCNAME   PGDATA                        PGLOG                                   
   myinst     20502    5432      /opt/sequoiasqloltp/pg_data   /opt/sequoiasqloltp/pg_data/myinst.log     
   Total: 1; Run: 1
   ```

4. 检查 SequoiaSQL 是否启动成功

   ```lang-javascript
   $ netstat -nap | grep 5432
   tcp   0   0 127.0.0.1:5432     0.0.0.0:*         LISTEN     20502/postgres
   unix  2   [ ACC ]   STREAM    LISTENING   40776754 20502/postgres     /tmp/.s.PGSQL.5432
   ```

5. 创建 SequoiaSQL 的 database

   ```lang-javascript
   $ bin/createdb -p 5432 foo
   ```

   进入 SequoiaSQL shell 环境

   ```lang-javascript
   $ bin/psql -p 5432 foo
   ```

##SequoiaSQL 开机自启动##

1. 安装 SequoiaSQL 时，会自动添加系统服务：Ssql-oltp。该服务在启动时，会自动拉起相关的实例，在实例进程异常退出时，也会自动拉起实例。

   >**Note:**   
   >系统服务名为 Ssql-oltp[i]，i 为小于 50 的数值或者为空。在安装包执行结束时，会打印出该版本对应的服务名。

   当添加一个新实例时，会自动加入 service 的管理中。

   ```lang-javascript
   $ bin/sdb_sql_ctl addinst myinst -D pg_data
   Adding instance myinst ...
   ok
   ```

2. 如果不想实例纳入服务的管理：

   ```lang-javascript
   $ bin/sdb_sql_ctl delfromsvc myinst
   Deleting instance myinst from service ...
   ok
   ```
   
   或者在添加实例的时候指定参数--addtosvc：

   ```lang-javascript
   $ bin/sdb_sql_ctl addinst myinst -D pg_data --addtosvc=false
   Adding instance myinst ...
   ok
   ```

3. 添加实例到服务

   ```lang-javascript
   $ bin/sdb_sql_ctl addtosvc myinst
   Adding instance myinst to service ...
   ok
   ```   

4. 查看服务运行状态

   ```lang-javascript
   $ service Ssql-oltp status
   Status of service Ssql-oltp: 
   running. (PID: 14756)
   ```

5. 启停服务

   ```lang-javascript
   $ service Ssql-oltp stop
   Stoping service Ssql-oltp ...
   ok.
   ```
   
   ```lang-javascript
   $ service Ssql-oltp start
   Starting service Ssql-oltp ...
   ok. (PID: 4822)
   ```