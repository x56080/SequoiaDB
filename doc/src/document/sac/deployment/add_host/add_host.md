
添加主机流程共3步骤：

| 步骤 | 说明 |
| ---- | ------------- |
| [扫描主机](sac/deployment/add_host/scan_host.md)    | 扫描主机，检查主机能正常通过SSH访问。 |
| [添加主机](sac/deployment/add_host/add_host.md)     | 获取主机硬件和系统信息。 |
| [安装主机](sac/deployment/add_host/install_host.md) | 执行主机安装包进行安装。 |

1. 系统会自动检查主机是否符合安装业务的要求。

   ![添加主机](sac/deployment/add_host/add_host_1.jpg)

2. 符合条件的主机，在左侧主机列表都会自动打钩选上。

   > **Note:**  
   > ubuntu-test主机是提供OM服务，不推荐在OM服务的主机上安装业务。  
   > 因为是演示环境，所以把ubuntu-test手工选上。

3. 在 **主机配置**，可以选择磁盘，容量不足和网络盘是禁止选择的，符合条件的磁盘都会默认选中。  
   选中的磁盘，将会在配置SequoiaDB业务时，自动分配SequoiaDB节点到磁盘上，如果不希望节点安装在某个磁盘上，在这里取消选中该磁盘即可。

4. 点击底部的 **下一步** 按钮，进入安装主机页面。

   ![添加主机](sac/deployment/add_host/add_host_2.jpg)

5. 开始安装主机，[点击查看](sac/deployment/add_host/install_host.md)。