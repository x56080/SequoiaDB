##安装 MySQL 实例组件##

###安装前准备###

- 使用 root 用户权限来安装 MySQL实例组件
- 检查 MySQL 实例组件产品软件包是否与 SequoiaDB 版本一致 
- 如果需要图形界面模式安装，请确保 X Server 服务正在运行

###安装步骤###

**说明：**

（1）产品包名字以 sequoiasql-mysql-3.2-linux_x86_64-enterprise-installer.run 为例。

（2）安装过程中若输入有误，可按ctrl+退格键进行删除。

（3）步骤以命令行方式进行介绍，图形界面按照图像向导提示完成。

- 运行安装程序  
  
  ```lang-bash
  # ./sequoiasql-mysql-3.2-linux_x86_64-enterprise-installer.run --mode text
  ```

  >**Note:**   
  >执行安装包不添加参数--mode，则进入图形界面。  

- 程序提示选择向导语言，输入2，选择中文

  ```
  Language Selection
  Please select the installation language
  [1] English - English
  [2] Simplified Chinese - 简体中文
  Please choose an option [1] :2
  ```

- 输入安装路径后按回车（默认安装在 /opt/sequoiasql/mysql ）

  ```
  ----------------------------------------------------------------------------
  由BitRock InstallBuilder评估本所建立
  
  欢迎来到 SequoiaSQL MySQL Server 安装程序

  ------------------------------------------------------------
  请指定 SequoiaSQL MySQL Server 将会被安装到的目录
  安装目录 /opt/sequoiasql/mysql
  ```

- 提示输入用户名和用户组（默认创建 sdbadmin 用户和 sdbadmin_group 用户组）

  ```
  ------------------------------------------------------------
  数据库管理用户配置
  配置用于启动 SequoiaSQL-MySql 的用户名、用户组和密码
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
  设定现在已经准备将 SequoiaSQL MySQL Server 安装到您的电脑.
  您确定要继续? [Y/n]: 
  ```
  
- 安装完成

  ```
  正在安装 SequoiaSQL MySQL Server 于您的电脑中，请稍候。
  安装中
  0% ______________ 50% ______________ 100%
  ########################################
  
  ------------------------------------------------------------
  安装程序已经完成安装 SequoiaSQL MySQL Server 于你的电脑中.
  ```

##部署 MySQL 实例组件##

1. 切换用户和目录

   ```lang-bash
    $ su - sdbadmin
    $ cd /opt/sequoiasql/mysql
   ```

2. 添加实例

   指定实例名为myinst，该实例名映射相应的数据目录和日志路径，用户可以根据自己需要指定不同的实例名，实例默认端口号为3306。

   ```lang-bash
   $ bin/sdb_sql_ctl addinst myinst -D database/3306/
   ```

   若端口号3306被占用，用户可以使用-p参数指定实例端口号：

   ```lang-bash
   $ bin/sdb_sql_ctl addinst myinst -D database/3316/ -p 3316
   ```

   查看实例：

   ```lang-bash
   $ bin/sdb_sql_ctl listinst
   NAME      SQLDATA                                  SQLLOG
   myinst     /opt/sequoiasql/mysql/database/3306/    /opt/sequoiasql/mysql/myinst.log
   Total: 1
   ```

3. 启动实例

   ```lang-bash
   $ bin/sdb_sql_ctl start myinst
   Starting instance myinst ...
   ok (PID: 25174)
   ```

4. 查看实例状态

   ```lang-bash
   $ bin/sdb_sql_ctl status
   INSTANCE   PID        SVCNAME    SQLDATA                                 SQLLOG            
   myinst     25174      3306       /opt/sequoiasql/mysql/database/3306/    /opt/sequoiasql/mysql/myinst.log        
   Total: 1; Run: 1
   ```

5. 停止实例

   ```lang-bash
   $ bin/sdb_sql_ctl stop myinst
   Stoping instance myinst (PID: 25174) ...
   ok
   ```

##MySQL 实例组件开机自启动##

 安装 MySQL 实例组件时，会自动添加系统服务：sequoiasql-mysql。该服务在启动时，会自动拉起相关的实例。在实例进程异常退出时，也会自动拉起实例。

   >**Note:**   
   >系统服务名为 sequoiasql-mysql[i]，i 为小于 50 的数值或者为空。

   当添加一个新实例时，会自动加入 service 的管理中。

   ```lang-bash
   $ bin/sdb_sql_ctl addinst myinst -D database/3306/
   Adding instance myinst ...
   ok
   ```

