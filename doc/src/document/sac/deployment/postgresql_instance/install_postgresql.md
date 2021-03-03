1. 进入 **部署 - 数据库实例** 页面。

  ![数据库实例](sac/deployment/postgresql_instance/add_pg_1.png)

2. 点击 **添加实例 - 创建实例**，实例类型选择 **PostgreSQL**， 点击 **确定** 按钮。

  ![创建 PostgreSQL 实例](sac/deployment/postgresql_instance/add_pg_2.png)

3. 进入 **配置实例** 页面，可以修改 PostgreSQL 的配置，点击 **下一步** 按钮。

  > **Note：**  
  > 如果出现 **安装路径** 和 **系统管理员** 配置项，说明这台主机没有安装 PostgreSQL 包。  
  > **安装路径** 是指 PostgreSQL 包安装在主机中的路径。  
  > **系统管理员** 是指主机的管理员账号，安装 PostgreSQL 包需要管理员权限，演示的 sdbserver1 的系统管理员账号是 admin。

  ![安装SequoiaSQL-PostgreSQL](sac/deployment/postgresql_instance/add_pg_3.png)

4. 安装实例完成。

  ![安装SequoiaSQL-PostgreSQL](sac/deployment/postgresql_instance/add_pg_4.png)

> **Note：**  
> 创建 PostgreSQL 实例后，可以给实例添加分布式存储，[点击查看](sac/deployment/postgresql_instance/add_postgresql_storage.md)。  
> 可以在线操作 PostgreSQL 实例，[点击查看](sac/postgresql_data/database.md)。