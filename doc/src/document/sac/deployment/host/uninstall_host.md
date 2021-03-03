
> **Note:**  
> 删除主机必须确保主机上没有存储集群和数据库实例：  
> 1. 如果手工删除了存储集群的节点，先同步配置再删除主机 [点击查看](sac/deployment/distributed_storage/sync_storage.md)  
> 2. 如果需要删除存储集群 [点击查看](sac/deployment/distributed_storage/uninstall_storage.md)  
> 3. 如果需要删除 MySQL 数据库实例 [点击查看](sac/deployment/mysql_instance/uninstall_mysql.md)  
> 4. 如果需要删除 PostgreSQL 数据库实例 [点击查看](sac/deployment/postgresql_instance/uninstall_postgresql.md)

1. 进入 **部署 - 主机** 页面。

   ![删除主机](sac/deployment/remove_host/uninstall_host_1.png)

2. 选择要删除的主机，可以点击 **全选** 按钮选择全部。

   ![删除主机](sac/deployment/remove_host/uninstall_host_2.png)

3. 点击 **删除主机 - 卸载主机**。

   ![删除主机](sac/deployment/remove_host/uninstall_host_3.png)

4. 等待任务完成。

   ![删除主机](sac/deployment/remove_host/uninstall_host_4.png)

5. 卸载完成。

   ![删除主机](sac/deployment/remove_host/uninstall_host_5.png)