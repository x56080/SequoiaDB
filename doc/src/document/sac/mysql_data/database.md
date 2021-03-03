**MySQL 实例操作** 页面可以进行创建数据库、删除数据库、创建数据表、删除数据表等操作。

创建数据库
---

1. 点击导航 **数据 - 数据库实例** 的名字，进入 MySQL 实例操作页面。

  ![SequoiaSQL-MySQL数据库](sac/mysql_data/database_1.png)

2. 点击 **创建数据库**，填写 **数据库名**，点击 **确定** 按钮。

  ![创建数据库](sac/mysql_data/database_2.png)

创建数据表
---

创建数据表可以选择存储引擎，默认为 **SequoiaDB** 引擎，确保数据表创建成功请提前查看是否已经[添加实例存储](sac/deployment/mysql_instance/add_mysql_storage.md)。

点击 **创建数据表**，填写好参数，点击 **确定** 按钮。

  ![创建数据表](sac/mysql_data/database_3.png)


> **Note:**  
> 创建完数据表后，可点击数据表名进入[数据操作](sac/mysql_data/record.md)。  
> 成功创建数据表时会在对应的 SequoiaDB 存储集群创建集合空间和集合。

删除数据库
---

点击 **删除数据库**，选择需要删除的数据库，点击 **确定** 按钮。

![删除数据库](sac/mysql_data/database_4.png)

删除数据表
---

从数据表列表中，点击需要删除的数据表表 **X** 按钮，点击 **确定** 删除。

> **Note:**  
> 系统表不可被删除。

![删除数据表](sac/mysql_data/database_5.png)

