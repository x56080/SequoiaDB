**减容操作** 可以删除存储集群的一个或多个节点。

1. 进入 **部署 - 分布式存储** 页面。

   ![服务选择](sac/deployment/distributed_storage/shrink_1.png)

2. 点击 **存储集群操作 - 减容**。

   ![节点选择](sac/deployment/distributed_storage/shrink_2.png)

3. 选择要减容的存储集群。

   ![节点选择](sac/deployment/distributed_storage/shrink_3.png)

4. 演示的是3个副本的环境。

   ![节点选择](sac/deployment/distributed_storage/shrink_4.png)

5. 演示所有分区组从3个副本改为单副本。

   > **Note:**  
   > 1. 如果删除整个数据组，要确保该组没有数据，否则该数据组将无法全部删除，会留下一个节点。  
   > 2. 为了保证存储集群正常工作，必须保留至少一个协调节点和一个编目节点。

   ![节点选择](sac/deployment/distributed_storage/shrink_5.png)

6. 等待任务。

   ![节点选择](sac/deployment/distributed_storage/shrink_6.png)

7. 任务完成。

   ![节点选择](sac/deployment/distributed_storage/shrink_7.png)