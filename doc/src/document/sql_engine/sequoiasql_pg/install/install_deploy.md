##安装 PostgreSQL 实例组件##

###安装前准备###

- 使用 root 用户权限来安装 PostgreSQL 实例组件
- 检查 PostgreSQL 实例组件产品软件包是否与 SequoiaDB 版本一致 
- 如果需要图形界面模式安装，请确保 X Server 服务正在运行

###安装步骤###

**说明：**

（1）产品包名字以 sequoiasql-postgresql-3.2-x86_64-enterprise-installer.run 为例。

（2）安装过程中若输入有误，可按ctrl+退格键进行删除。

（3）步骤以命令行方式进行介绍，图形界面按照图像向导提示完成。

- 运行安装程序  
  
  ```lang-bash
  # ./sequoiasql-postgresql-3.2-x86_64-enterprise-installer.run --mode text
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

- 输入安装路径后按回车（默认安装在 /opt/sequoiasql/postgresql ）

  ```
  ----------------------------------------------------------------------------
  由BitRock InstallBuilder评估本所建立
  
  欢迎来到 SequoiaSQL PostgreSQL Server 安装程序

  ------------------------------------------------------------
  请指定 SequoiaSQL PostgreSQL Server 将会被安装到的目录
  安装目录 [/opt/sequoiasql/postgresql]:
  ```

- 提示输入用户名和用户组（默认创建 sdbadmin 用户和 sdbadmin_group 用户组），该用户名用于运行 SequoiaSQL PostgreSQL 服务

  ```
  ------------------------------------------------------------
  数据库管理用户配置
  配置用于启动 SequoiaSQL PostgreSQL 的用户名、用户组和密码
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
  设定现在已经准备将 SequoiaSQL PostgreSQL Server 安装到您的电脑.
  您确定要继续? [Y/n]: 
  ```
  
- 安装完成

  ```
  正在安装 SequoiaSQL PostgreSQL Server 于您的电脑中，请稍候。
  安装中
  0% ______________ 50% ______________ 100%
  ########################################
  添加了系统服务: sequoiasql-postgresql.
  #
  ------------------------------------------------------------
  安装程序已经完成安装 SequoiaSQL PostgreSQL Server 于你的电脑中.
  ```

##部署 PostgreSQL 实例组件 ##

1. 检查端口是否被占用

   SequoiaSQL PostgreSQL 默认启动端口为5432,检查端口是否被占用。

   ```lang-bash
   $ sudo netstat -nap | grep 5432
   ```

2. 切换用户和目录

   ```lang-bash
   $ su - sdbadmin
   $ cd /opt/sequoiasql/postgresql
   ```


3. 创建实例

   指定实例名为myinst，该实例名映射相应的数据目录和日志路径，用户可以根据自己需要指定不同的实例名。

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D database/5432/
   ```

   若端口号5432被占用，用户可以使用-p参数指定实例端口号：

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D database/5442/ -p 5442
   ```

   查看实例：

   ```lang-bash
   $ bin/sdb_pg_ctl listinst
   NAME       PGDATA                                       PGLOG
   myinst     /opt/sequoiasql/postgresql/database/5432/    /opt/sequoiasql/postgresql/myinst.log
   Total: 1
   ```

4. 启动实例进程

   ```lang-bash
   $ bin/sdb_pg_ctl start myinst
   Starting instance myinst ...
   ok (PID: 28115)
   ```
   
    查看实例状态
   
    ```lang-bash
   $ bin/sdb_pg_ctl status
   INSTANCE   PID        SVCNAME    PGDATA                                   PGLOG
   myinst     28115      5432       /opt/sequoiasql/postgresql/database/5432/ /opt/sequoiasql/postgresql/myinst.log
   Total: 1; Run: 1
    ```

5. 创建 SequoiaSQL PostgreSQL 的 database

   ```lang-bash
   $ bin/sdb_pg_ctl createdb foo myinst
   ```

   进入 SequoiaSQL PostgreSQL shell 环境

   ```lang-bash
   $ bin/psql -p 5432 foo
   ```

##PostgreSQL实例组件系统服务##

安装 PostgreSQL 实例组件时，会自动添加 sequoiasql-postgresql 系统服务。该服务会在系统启动的时候自动运行。该服务是 PostgreSQL 实例的守护进程。它能在机器启动时，自动启动相关的 PostgreSQL 实例；它能实时重启异常退出的 PostgreSQL 实例进程。

   > **Note:**  
   > 
   > 一个安装对应一个 sequoiasql-postgresql 服务，一台机器上存在多个安装时，系统服务名为 sequoiasql-postgresql[i]，i 为小于 50 的数值或者为空。


用户可通过 service 命令管理 sequoiasql-postgresql 系统服务。

- 如果需要查看服务的运行状态，可使用如下命令：

   ```lang-bash
   $ sudo service sequoiasql-postgresql status
   ```

- 如果需要停止服务，可使用如下命令：

   ```lang-bash
   $ sudo service sequoiasql-postgresql stop
   ```

- 如果需要启动服务，可使用如下命令：

   ```lang-bash
   $ sudo service sequoiasql-postgresql start
   ```

用户添加的新实例会自动加入 sequoiasql-postgresql 系统服务的管理中。

- 如果需要将指定实例从服务的管理中剔除，可使用如下命令：

   ```lang-bash
   $ bin/sdb_pg_ctl delfromsvc myinst
   ```
   
   或者在添加实例的时候指定参数--addtosvc

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D database/5432/ --addtosvc=false
   ```

- 如果需要将被剔除的实例重新纳入服务的管理，可使用如下命令：

   ```lang-bash
   $ bin/sdb_pg_ctl addtosvc myinst
   ```