**扩容操作** 在存储集群新增一个或多个节点。

1. 进入 **部署 - 分布式存储** 页面。

   ![服务选择](sac/deployment/distributed_storage/extend_1.png)

2. 点击 **存储集群操作 - 扩容**。

   ![服务选择](sac/deployment/distributed_storage/extend_2.png)

3. 选择要扩容的存储集群，点击 **确定** 按钮。

   ![服务选择](sac/deployment/distributed_storage/extend_3.png)

4. 选择 **扩容模式**，请根据实际需求选择，演示将添加副本数，从1个副本增加到3个副本。

   * **添加分区组**，设置 **分区组数** 和 **副本数**，点击 **下一步**按钮。

     ![水平扩容](sac/deployment/distributed_storage/extend_4.png)

   * **添加副本数**，设置每个分区组需要添加的副本数，点击 **下一步**按钮。

     ![垂直扩容](sac/deployment/distributed_storage/extend_5.png)

5. 在 **修改服务** 页面，可以修改新增节点的配置，然后点击 **下一步** 按钮。

   > **Note:**  
   > 1. 批量修改节点配置 **数据路径** 和 **服务名** 支持特殊规则来简化修改。规则可以点击页面 **提示** 的 **帮助**。  
   > 2. 批量修改节点配置时，如果值为空，那么代表该参数的值不修改。

   ![修改服务](sac/deployment/distributed_storage/extend_6.png)

6. 等待任务完成。

   ![修改服务](sac/deployment/distributed_storage/extend_7.png)

7. 任务完成。

   ![修改服务](sac/deployment/distributed_storage/extend_8.png)
