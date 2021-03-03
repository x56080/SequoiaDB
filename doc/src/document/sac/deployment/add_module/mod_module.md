
添加业务流程共3步骤：

| 步骤 | 说明 |
| ---- | ------------- |
| [配置业务](sac/deployment/add_module/config_module.md)  | 配置业务部署模式、主机、节点分布。 |
| [修改业务](sac/deployment/add_module/mod_module.md)     | 修改业务的配置参数。 |
| [安装业务](sac/deployment/add_module/install_module.md) | 安装业务的服务。 |

1. 进入修改业务页面，这里可以修改业务的配置，这里演示的是集群模式的配置。

   ![修改业务](sac/deployment/add_module/modify_module_1.jpg)

2. 一般自动生成的配置不需要修改就能直接下一步安装。

3. 这一步不是必须做的操作，因为演示的主机都只有1个磁盘，mount在根目录（/ 路径），因此需要修改数据路径。
点击 **全选**，然后点击 **批量修改节点**，把 **数据路径** 改成 /opt/sequoiadb/database/[role]/[svcname]，点击 **确定**。

   ![修改业务](sac/deployment/add_module/modify_module_2.jpg)

4. 这是修改后的配置。

   ![修改业务](sac/deployment/add_module/modify_module_3.jpg)

5. 点击底部的 **下一步** 按钮，进入安装业务页面。

6. 开始安装业务，[点击查看](sac/deployment/add_module/install_module.md)。

> **Note:**  
> 1. 如果要开启事务，必须所有节点都开启。  
> 2. 批量修改节点配置 **数据路径** 和 **服务名** 支持特殊规则来简化修改。规则可以点击页面 **提示** 的 **帮助**。  
> 3. 批量修改节点配置时，如果值为空，那么代表该参数的值不修改。

####导入导出配置####

**v2.8版本**新增SequoiaDB集群模式下导入导出配置，支持JSON和XML格式。

点击 **编辑配置** 按钮，出现编辑配置的窗口。

![修改业务](sac/deployment/add_module/modify_module_4.jpg)