1. 进入 **部署 - 数据库实例** 页面。

   ![数据库实例](sac/deployment/mysql_instance/add_mysql_1.png)

2. 点击 **添加实例 - 创建实例**，实例类型选择 **MySQL**， 点击 **确定** 按钮。

   ![创建 MySQL 实例](sac/deployment/mysql_instance/add_mysql_2.png)

3. 进入 **配置实例** 页面，可以设置 MySQL 实例的配置，点击 **下一步** 按钮。

   > **Note：**  
   > **安装路径** 是指 MySQL 包安装在主机中的路径。  
   > **系统管理员** 是指主机的管理员账号，安装 MySQL 包需要管理员权限，演示的 sdbserver1 的系统管理员账号是 admin。  
   > **MySQL 账号** 是指用于访问 MySQL 数据库的账号，如果用户名是 root，则修改默认账号 root 的密码。  

   * 没有安装 MySQL 包的 sdbserver1 主机，会出现 **安装路径** 和 **系统管理员** 配置项：

     ![创建 MySQL 实例](sac/deployment/mysql_instance/add_mysql_3.png)

   * 已经安装 MySQL 包的 sdbserver2 主机：

     ![创建 MySQL 实例](sac/deployment/mysql_instance/add_mysql_4.png)

4. 安装实例完成。

   ![创建 MySQL 实例](sac/deployment/mysql_instance/add_mysql_5.png)

> **Note：**  
> 创建 MySQL 实例后，可以给实例添加分布式存储，[点击查看](sac/deployment/mysql_instance/add_mysql_storage.md)。  
> 可以在线操作 MySQL 实例，[点击查看](sac/mysql_data/database.md)。