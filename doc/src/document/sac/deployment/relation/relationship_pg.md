**关联服务** 是指两个不同类型的服务对接，例如：  
SequoiaSQL-PostgreSQL 服务关联 SequoiaDB 服务；  
SequoiaSQL-MySQL 服务关联 SequoiaDB 服务。

这里演示 SequoiaSQL-PostgreSQL 服务关联 SequoiaDB 服务。

> **Note：**  
> 使用关联服务需要 SAC 有 **SequoiaDB 服务** 和 **SequoiaSQL-PostgreSQL 服务**。  
> 在 SAC 创建 SequoiaDB 服务，[点击查看](sac/deployment/add_sdb_module/config_module.md)。  
> 在 SAC 创建 SequoiaSQL-PostgreSQL 服务，[点击查看](sac/deployment/install_postgresql.md)。


###创建关联

1. 点击左侧导航的 **部署**，进入部署页面。

   ![关联服务](sac/deployment/relation/pg_sdb_1.png)

2. 点击 **关联服务 - 创建关联**。

   ![关联服务](sac/deployment/relation/pg_sdb_2.png)

3. 在弹窗填写关联参数。

   > **Note：**
   > 关联服务参数说明：  
   > **关联名**：创建关联完成后的名字，全局唯一。  
   > **数据库**：选择 SequoiaSQL-PostgreSQL 要关联 SequoiaDB 的数据库。  
   > **preferedinstance**：指定 SequoiaSQL-PostgreSQL 访问SequoiaDB 数据节点时，优先连接哪种角色的数据节点，默认为’a’，可输入参数 m / s / a / 1-7，分别表示 master / slave / anyone / node1-node7。  
   > **transaction**：设置 SequoiaDB 是否开启事务，默认为 off。开启为 on。  
   > **选择被关联节点**：选择关联 SequoiaDB 服务的 Coord 节点，默认所有 Coord 节点。 

   ![关联服务](sac/deployment/relation/pg_sdb_3.png)

   ![关联服务](sac/deployment/relation/pg_sdb_4.png)

4. 创建关联成功。

   ![关联服务](sac/deployment/relation/pg_sdb_5.png)

###解除关联

1. 点击左侧导航的 **部署**，进入部署页面。

   ![关联服务](sac/deployment/relation/d_pg_sdb_1.png)

2. 点击 **关联服务 - 解除关联**。

   ![关联服务](sac/deployment/relation/d_pg_sdb_2.png)

3. 选择要解除的 **关联名**。

   ![关联服务](sac/deployment/relation/d_pg_sdb_3.png)

4. 解除关联成功。

   ![关联服务](sac/deployment/relation/d_pg_sdb_4.png)
