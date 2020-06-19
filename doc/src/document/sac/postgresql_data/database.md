**PostgreSQL 实例操作** 页面可以进行创建数据库、删除数据库、创建数据表、删除数据表等操作。

创建数据库
---

1. 点击导航 **数据 - 数据库实例** 的名字，进入 PostgreSQL 实例操作页面。

  ![SequoiaSQL-PostgreSQL数据库](sac/postgresql_data/database_1.png)

2. 点击 **创建数据库**，填写 **数据库名**，点击 **确定** 按钮。

  ![创建数据库](sac/postgresql_data/database_2.png)

删除数据库
---

点击 **删除数据库**，选择要删除的数据库，点击 **确定** 按钮。

> **Note:**  
> 无法删除当前选择的数据库。通过 SAC 删除数据库，至少保留有一个数据库。

![删除数据库](sac/postgresql_data/database_3.png)

创建数据表
---

创建数据表可以选择 **普通表** 和 **外部表**，创建外部表需要给数据库实例 [添加存储集群](sac/deployment/postgresql_instance/add_postgresql_storage.md)。

点击 **创建数据表**，填写好参数，点击 **确定** 按钮。

> **Note：**  
> 创建数据表后，点击数据表名进入 [数据操作](sac/postgresql_data/record.md)  
> 创建外部表不可定义主键和唯一键。

![创建数据表](sac/postgresql_data/database_4.png)

删除数据表
---

从数据表列表中，点击需要删除的数据表 **X** 按钮，点击 **确定** 删除。

> **Note:**  
> 系统表不能删除。

![删除数据表](sac/postgresql_data/database_5.png)

