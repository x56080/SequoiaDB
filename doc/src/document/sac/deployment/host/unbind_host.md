**解绑主机** 是把主机从 SAC 管理中移除，不会卸载该主机。

> **Note:**  
> 解绑主机必须确保主机上没有存储集群和数据库实例：  
> 1. 如果手工删除了存储集群的节点，先同步配置再删除主机 [点击查看](sac/deployment/distributed_storage/sync_storage.md)  
> 2. 如果需要删除存储集群 [点击查看](sac/deployment/distributed_storage/uninstall_storage.md)，或者移除存储集群 [点击查看](sac/deployment/distributed_storage/unbind_storage.md)  
> 3. 如果需要删除 MySQL 数据库实例 [点击查看](sac/deployment/mysql_instance/uninstall_mysql.md)，或者移除 MySQL 数据库实例 [点击查看](sac/deployment/mysql_instance/unbind_mysql.md)    
> 4. 如果需要删除 PostgreSQL 数据库实例 [点击查看](sac/deployment/postgresql_instance/uninstall_postgresql.md)，或者移除 PostgreSQL 数据库实例 [点击查看](sac/deployment/postgresql_instance/unbind_postgresql.md)

1. 进入 **部署 - 主机** 页面。

   ![解绑主机](sac/deployment/remove_host/unbind_host_1.png)

2. 选择要解绑的主机，可以点击 **全选** 按钮选择全部。

   ![解绑主机](sac/deployment/remove_host/unbind_host_2.png)

3. 点击 **删除主机-解绑主机** 按钮。

   ![解绑主机](sac/deployment/remove_host/unbind_host_3.png)

4. 解绑完成，点击 **确定**。

   ![解绑主机](sac/deployment/remove_host/unbind_host_4.png)

5. 演示的三台主机都解绑了。

   ![解绑主机](sac/deployment/remove_host/unbind_host_5.png)