部署包是将指定的包部署到指定的主机中。目前支持部署 MySQL 包和 PostgreSQL 包。  
演示以 MySQL 包为例。

> **Note:**  
> 部署包之前需要将 run 包放在 SAC 主机安装路径的 packet 目录中，默认：/opt/sequoiadb/packet。

1. 在 **部署** 页面勾选需要部署包的主机，点击 **主机操作 - 部署包** 进入配置页面。

   ![部署包](sac/deployment/host/deploy_package_1.png)

2. 填写好参数之后，点击下一步进行安装。

   > **Note:**  
   > **强制安装：**  
   > 选择 false 时，会忽略已经安装的主机。  
   > 选择 true 时，会在选择的主机上重新安装，并且强制重启的对应服务。
   >
   > **用户名：**  
   > 支持 root 和 sudo 权限的用户。

   ![部署包](sac/deployment/host/deploy_package_2.png)

3. 安装完成。

   ![部署包](sac/deployment/host/deploy_package_3.png)

4. 点击 **完成**，回到 **部署** 页面，查看主机，可以看见主机已经安装 MySQL。

   ![部署包](sac/deployment/host/deploy_package_4.png)