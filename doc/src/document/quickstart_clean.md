## 一键清理 ##

一键清理仅针对于[快速入门](quickstart.md)使用。

###清理前准备###

+ 清理过程需要使用操作系统 root 用户权限

+ 请确保清理脚本clean.sh具有可执行权限

###清理步骤###

- 运行清理脚本，通过询问用户的方式清除当前主机上 setup.sh 安装的所有 SequoiaDB、MySQL 实例组件以及 PostgreSQL 实例组件和数据

  ```lang-bash
  # ./clean.sh
  ```

- 程序提示是否清除安装路径下的 SequoiaDB，默认是清理，输入N不清理

  ```
   clean /opt/sequoiadb sequoiadb Y/n: 
   begin to uninstall sequoiadb
   /opt/sequoiadb/uninstall --mode unattended
   ok
   rm -rf /opt/sequoiadb/database/catalog/11800/
   rm -rf /opt/sequoiadb/database/coord/11810/
   rm -rf /opt/sequoiadb/database/data/11820/
   rm -rf /opt/sequoiadb/database/data/11830/
   rm -rf /opt/sequoiadb/database/data/11840/
   begin to clean install dir
   rm -rf /opt/sequoiadb
   ok
  ```

- 程序提示是否清除安装路径下的 MySQL 实例组件，默认是清理，输入N不清理

  ```
   clean /opt/sequoiasql/mysql sequoiasql-mysql Y/n: 
   begin to uninstall sequoiasql-mysql
   /opt/sequoiasql/mysql/uninstall --mode unattended
   ok
   rm -rf /opt/sequoiasql/mysql/database/3306
   rm -rf /opt/sequoiasql/mysql/myinst.log
   begin to clean install dir
   rm -rf /opt/sequoiasql/mysql
   ok
  ```

   > **Note:**
   > 
   > * 用户可以指定 --sdb 只清除 SequoiaDB 的安装和数据
   > * 用户可以指定 --mysql 只清除 MySQL 实例组件的安装和数据
   > * 用户可以指定 --pg 只清除 PostgreSQL 实例组件的的安装和数据
   > * 用户可以指定 --force 不再询问用户，直接清除通过setup.sh安装的所有软件
   > * 用户可以指定 --local 清除当前机器存在的所有 SequoiaDB、MySQL 实例组件以及 PostgreSQL 实例组件，默认只清理通过setup.sh安装的