**添加已有的存储集群** 可以把存储集群添加到 SAC 中管理。

1. 进入 **部署 - 分布式存储** 页面。

   ![发现服务](sac/deployment/distributed_storage/append_sdb_1.png)

2. 点击 **添加存储集群 - 添加已有的存储集群** 按钮。

   ![发现服务](sac/deployment/distributed_storage/append_sdb_2.png)

3. **类型** 选择 **SequoiaDB**，点击 **确定** 按钮。

   ![发现服务](sac/deployment/distributed_storage/append_sdb_3.png)

4. **地址** 支持 IP 和 主机名，填写 SequoiaDB 的协调节点地址，服务名填写协调节点的端口号，点击 **确定** 按钮。

   > **Note：**  
   > 如果 SequoiaDB 创建了鉴权，需要填写 **数据库用户名** 和 **数据库密码**。

   ![发现SequoiaDB](sac/deployment/distributed_storage/append_sdb_4.png)

5. 如果 SequoiaDB 节点的主机不都在 SAC 管理中，会有提示弹窗；点击 **是**，进入 **安装主机**，安装完成后，进入下一步；如果没有提示弹窗，跳过当前步骤。

   ![发现服务](sac/deployment/distributed_storage/append_sdb_5.png)

6. 完成，在这个页面可以查看 SequoiaDB 的节点信息。

   ![发现SequoiaDB](sac/deployment/distributed_storage/append_sdb_6.png)
