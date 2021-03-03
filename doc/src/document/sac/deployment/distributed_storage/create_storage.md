
1. 在 **部署 - 分布式存储** 页面，点击 **添加存储集群 - 创建存储集群**，点击 **确定**。  

   ![配置服务](sac/deployment/distributed_storage/add_sdb_module_1.png)

2. 配置服务，根据实际需求配置。

   ![配置服务](sac/deployment/distributed_storage/add_sdb_module_2.png)

3. 点击 **选择安装服务的主机**，可以指定主机安装服务，默认已经全选。

   ![配置服务](sac/deployment/distributed_storage/add_sdb_module_3.png)

4. 演示将会使用3台主机安装 SequoiaDB 集群，1个副本，3个分区组，点击 **下一步** 按钮。

   ![配置服务](sac/deployment/distributed_storage/add_sdb_module_4.png)

5. 演示的是 SequoiaDB 集群模式，这里可以修改每个节点配置。修改完成后，点击 **下一步** 按钮。

   > **Note:**  
   > 1. 批量修改节点配置 **数据路径** 和 **服务名** 支持特殊规则来简化修改。规则可以点击页面 **提示** 的 **帮助**。  
   > 2. 批量修改节点配置时，如果值为空，那么代表该参数的值不修改。

   ![修改服务](sac/deployment/distributed_storage/add_sdb_module_5.png)

6. 开始创建节点。

   ![安装服务](sac/deployment/distributed_storage/add_sdb_module_7.png)

7. 安装完成。

   ![安装服务](sac/deployment/distributed_storage/add_sdb_module_8.png)