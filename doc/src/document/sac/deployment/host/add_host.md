> **Note：**  
> 先 **创建集群** 才可以在这个 **集群** 中添加主机 [点击查看](sac/deployment/create_cluster.md)。

1. 在 **部署 - 主机** 页面点击 **添加主机** 按钮。

   ![添加主机](sac/deployment/host/add_host_1.png)

2. 在 **IP地址/主机名** 输入要扫描主机的IP地址或主机名。

   扫描主机支持4种输入方式：
   * 普通格式：```sdbserver1``` 或者 ```192.168.1.101```
   * 区间格式：```sdbserver[1-3]``` 或者 ```192.168.1.[101-103]```
   * 逗号分隔：```sdbserver1, sdbserver2, sdbserver3``` 或者 ```192.168.1.101, 192.168.1.102, 192.168.1.103```
   * 换行分隔：

     ```
     sdbserver1
     sdbserver2
     sdbserver3
     ```

     或者

     ```
     192.168.1.101
     192.168.1.102
     192.168.1.103
     ```

   ![扫描主机](sac/deployment/host/add_host_2.png)

3. **用户名** 必须是 root，安装需要 root 权限，输入 root 密码。

   > **Note:**  
   > 设置 Linux 系统 root 密码：

   > 1. 执行 ```sudo passwd root```
   > 2. 根据提示输入 root 密码

   > SSH 允许 root 使用密码登陆：

   > 1. 修改 ```/etc/ssh/sshd_config```
   > 2. 找到配置项 ```PermitRootLogin``` 和 ```PasswordAuthentication```，把值都改成 ```yes```
   > 3. 重启 SSH 服务， ```service sshd restart```

4. **SSH 端口** 根据实际端口修改（默认22），代理端口默认11790。

5. 点击 **扫描** 按钮。扫描成功后，点击 **下一步** 按钮。

   ![扫描主机](sac/deployment/host/add_host_3.png)

6. 系统会自动检查主机是否符合安装服务的要求，符合条件的主机，在左侧主机列表都会自动打钩选上。

   > **Note:**  
   > 演示环境 sdbserver1 主机是提供 OM 服务，默认不会选择。

   ![添加主机](sac/deployment/host/add_host_4.png)

7. 在 **主机配置**，可以选择磁盘，容量不足和网络盘是禁止选择的，符合条件的磁盘都会默认选中。  
   选中的磁盘，将会在配置服务时，自动分配服务的节点到磁盘上，如果不希望节点安装在某个磁盘上，在这里取消选中该磁盘即可。

8. 点击 **下一步** 按钮，开始安装。

9. 等待安装完成。

   ![安装主机](sac/deployment/host/add_host_5.png)

10. 安装完成。

   ![安装主机](sac/deployment/host/add_host_6.png)

