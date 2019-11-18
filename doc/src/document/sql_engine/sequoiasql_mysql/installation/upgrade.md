从低版本 MySQL 实例组件升级到较高版本。

##升级说明##

  - 只能支持向后兼容，同个版本也可升级，但不能从高版本升为低版本。
  - 升级不兼容 3.2 之前的版本， 3.2 之前的版本升级至 3.2 及 3.2 以后的版本需要手工升级。
  - 升级不会改动任何配置和数据。

##自动升级##

自动升级适用于从 3.2 版本或者是 3.2 以上的版本升级到更高版本的 MySQL 实例组件，使用 installmode 参数指定 upgrade 升级模式进行自动升级，以 sequoiasql-mysql-3.2.4-linux_x86_64-enterprise-installer.run 为例，升级步骤如下：

- 使用文本模式指定升级参数进行升级

  ```lang-bash
  # ./sequoiasql-mysql-3.2.4-linux_x86_64-installer.run --mode text --installmode upgrade
  ```

- 程序提示选择向导语言，输入2，选择中文

   ```
   Language Selection
   
   Please select the installation language
   [1] English - English
   [2] Simplified Chinese - 简体中文
   Please choose an option [1] : 2
   ```

- 显示安装协议，如果需要读取全部文件，输入2。输入1表示忽略阅读并同意协议。  

   ```
   ----------------------------------------------------------------------------
   由BitRock InstallBuilder评估本所建立

   欢迎来到 SequoiaSQL MySQL Server 安装程序

   ----------------------------------------------------------------------------
   GNU 通用公共授权
   第二版, 1991年6月
   著作权所有 (C) 1989，1991 Free Software Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
   允许每个人复制和发布本授权文件的完整副本，但不允许对它进行任何修改。
   
   [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
   [2] 查看详细的协议内容
   请选择一个选项 [1] : 
   ```

- 列出可升级的选项，选择1则升级 /opt/sequoiasql/mysql 目录下的安装 ，选择2则需要指定自定义路径，若指定的路径下存在安装则升级，若没有则安装 

   ```
   ----------------------------------------------------------------------------
   请指定 SequoiaSQL MySQL Server 将会被安装到的目录
   
      版本信息  安装目录

   [1] 3.2    /opt/sequoiasql/mysql
   [2] other option
   请选择一个选项 [1] : 
   ```
- 确认继续

   ```
   ----------------------------------------------------------------------------
   设定现在已经准备将 SequoiaSQL MySQL Server 安装到您的电脑.
   
   您确定要继续? [Y/n]: 
   ```

- 开始升级，升级过程中会显示检查列表

   ```
   ----------------------------------------------------------------------------
   正在安装 SequoiaSQL MySQL Server 于您的电脑中，请稍候.
   
    安装中
    0% ______________ 50% ______________ 100%
    开始升级
   **************************  检查列表 *************************************
   检查：在/etc/default/sequoiasql-mysql中获取用户名 ...... ok
   检查：安装目录/opt/sequoiasql/mysql不为空 ...... ok
   检查：旧版本 3.2 与新版本 3.2.4 兼容 ...... ok
   检查：磁盘空间足够 ...... ok
   检查：umask配置 ...... ok
   检查：用户sdbadmin存在，并获取用户组 ...... ok
   检查：相关进程已停止 ...... ok
   #########################################
   
   ----------------------------------------------------------------------------
   安装程序已经完成安装 SequoiaSQL MySQL Server 于你的电脑中.
   ```

- 升级完成

##手工升级##

手工升级只适用于 MySQL 实例组件从 3.2 以下的版本升级到 3.2 及其以上的版本。例如从 3.0.2 升级到 3.2 版本，且已存在端口号为 3306 路径为 /opt/sequoiasql/mysql/database/3306 的数据库实例，升级步骤如下：

- 以 root 用户登陆目标主机，卸载旧版本的 MySQL 实例组件；

   1. 进入安装路径，执行卸载；

     ```lang-bash
     # cd /opt/sequoiasql/mysql
     # ./uninstall --mode unattended
      ```
     >**Note:**  
     > 卸载不会清除数据目录以及安装路径下的配置文件 my.cnf

- 安装 3.2 版本的 MySQL 实例组件；

   1. 赋予安装包可执行权限，指定静默模式安装；

     ```lang-bash
     # chmod u+x sequoiadb-3.2-linux_x86_64-installer.run
     # ./sequoiadb-3.2-linux_x86_64-installer.run --mode unattended
     ```

- 迁移数据库实例合并实例配置项并启动实例；

   1. 切换目录和用户；

     ```lang-bash
     # cd /opt/sequoiasql/mysql
     # su sdbadmin
      ```

   2. 将旧的数据目录 database/3306 进行备份；

     ```lang-bash
     # mv database/3306 database/3306_bk
     ```

   3. 添加实例名为 mysqld3306 的数据库实例；

     ```lang-bash
     # bin/sdb_sql_ctl addinst mysqld3306 -p 3306 -D database/3306
     ```

   4. 停止实例；

     ```lang-bash
     # bin/sdb_sql_ctl stop mysqld3306
     ```

   5. 备份新的实例数据目录下的 database/3306/auto.cnf；

     ```lang-bash
     # mv database/3306/auto.cnf database/3306/auto.cnf_bk
     ```

   6. 将备份数据拷贝到新的实例目录下，并将上一步操作中备份的配置文件还原；

     ```lang-bash
     # cp -r database/3306_bk/* database/3306
     # mv database/3306/auto.cnf_bk database/3306/auto.cnf
    ```

   7. 将安装路径下 my.cnf 中 [mysqld3306] 的配置项合入 database/3306/auto.cnf 中的 [mysqld] 中；

   8. 启动实例；

     ```lang-bash
     # bin/sdb_sql_ctl start mysqld3306
     ```
