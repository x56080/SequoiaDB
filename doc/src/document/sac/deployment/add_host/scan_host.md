
添加主机流程共3步骤：

| 步骤 | 说明 |
| ---- | ------------- |
| [扫描主机](sac/deployment/add_host/scan_host.md)    | 扫描主机，检查主机能正常通过SSH访问。 |
| [添加主机](sac/deployment/add_host/add_host.md)     | 获取主机硬件和系统信息。 |
| [安装主机](sac/deployment/add_host/install_host.md) | 执行主机安装包进行安装。 |

> **Note:**  
> ubuntu的root账号默认是被禁用的，需要手工开启。  
> ubuntu开启root，执行命令 ```sudo passwd root```，按照提示输入root密码。

1. 在 部署首页-主机 页面点击 **添加主机** 按钮开始扫描主机。
   ![添加主机](sac/deployment/add_host/scan_host.jpg)

2. 在 **IP地址/主机名** 输入要扫描主机的IP地址或主机名。

   ![扫描主机](sac/deployment/add_host/scan_host_1.jpg)

   扫描主机支持4种输入方式。
   * 单机扫描：

      ![扫描主机](sac/deployment/add_host/scan_host_2.jpg)
   * 段扫描：段扫描不仅支持IP段，也支持主机名，如：ubuntu-test-[001-002]

      ![扫描主机](sac/deployment/add_host/scan_host_3.jpg)
   * 列表扫描：

      用逗号分隔。
      ![扫描主机](sac/deployment/add_host/scan_host_4.jpg)

      或者换行分隔
      ![扫描主机](sac/deployment/add_host/scan_host_5.jpg)

3. 用户名必须是root，安装需要root权限，如果系统禁止root登录，需要手工去开启。

4. 输入密码，SSH端口根据实际端口修改（默认22）。代理端口在这里不修改。

5. 点击 **扫描** 按钮。扫描成功后，点击底部的 **下一步** 按钮，进入添加主机页面。

   ![扫描主机](sac/deployment/add_host/scan_host_6.jpg)

6. 开始添加主机，[点击查看](sac/deployment/add_host/add_host.md)。