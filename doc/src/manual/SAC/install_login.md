本文档主要介绍如何安装 OM 服务及登录 SAC。SequoiaDB 巨杉数据库的可视化安装在多台服务器的情况下，只需要选择一台机器安装 OM 服务（SequoiaDB 管理中心进程），即可通过网页连接 OM 进行可视化安装部署集群。

## 下载安装包

用户可前往 [SequoiaDB 下载中心][download] 下载相应版本的安装包。

## 安装

### 安装前准备

   - 确保系统满足硬件和软件要求，并参照 [Linux 系统要求][system_requirement]和 [Linux 推荐配置][linux_suggest_setting]配置好主机名以及修改系统内核参数
   - 检查 SequoiaDB 产品软件包是否与 OS 系统配套
   - 如果需要图形界面安装，应确保 X Server 服务处于运行状态
   - 使用 root 用户权限来安装 SequoiaDB 数据库服务

### 安装步骤
     
以下安装过程将使用名称为 `sequoiadb-{version}-linux_x86_64-installer.run` 的产品包为示例。

> **Note:**
>
>* 安装步骤以命令行方式进行介绍，图形界面可按照图像向导提示完成。
>* SequoiaDB 安装向导需要的参数不接受非英文字符。


1. 使用 root 用户运行 `sequoiadb-{version}-linux_x86_64-installer.run` 包

   ```lang-bash
   # ./sequoiadb-{version}-linux_x86_64-installer.run --SMS true
   ```

2. 程序提示选择向导语言，输入2，选择中文

   ```lang-text
   Language Selection
   Please select the installation language
   [1] English - English
   [2] Simplified Chinese - 简体中文
   Please choose an option [1] :2
   ```

3. 显示安装协议，输入回车，默认忽略阅读；输入 2 则表示读取完整协议内容

   ```lang-text
   ------------------------------------------------------------
   由 BitRockInstallBuilder 评估本所建立
   ------------------------------------------------------------
   欢迎来到 SequoiaDB Server 安装程序


   重要信息：请仔细阅读

   下面提供了两个许可协议。

   1. SequoiaDB 评估程序的最终用户许可协议
   2. SequoiaDB 最终用户许可协议

   如果被许可方为了生产性使用目的（而不是为了评估、测试、试用“先试后买”或演示）获得本程序，单击下面的“接受”按钮即表示被许可方接受 SequoiaDB 最终用户许可协议，且不作任何修改。

   如果被许可方为了评估、测试、试用“先试后买”或演示（统称为“评估”）目的获得本程序：单击下面的“接受”按钮即表示被许可方同时接受（i）SequoiaDB 评估程序的最终用户许可协议（“评估许可”），且不作任何修改；和（ii）SequoiaDB 最终用户程序许可协议（SELA），且不作任何修改。

   在被许可方的评估期间将适用“评估许可”。

   如果被许可方通过签署采购协议在评估之后选择保留本程序（或者获得附加的本程序副本供评估之后使用），SequoiaDB 评估程序的最终用户许可协议将自动适用。

   “评估许可”和 SequoiaDB 最终用户许可协议不能同时有效；两者之间不能互相修改，并且彼此独立。

   这两个许可协议中每个协议的完整文本如下。

   评估程序的最终用户许可协议



   [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
   [2] 查看详细的协议内容
   请选择选项 [1] :
   ```
  
4. 是否同意协议，输入 y，表示同意协议

   ```lang-text
   ------------------------------------------------------------
   同意以上协议

   按 [Enter] 继续：

   您是否接受此软件授权协议？ [y/n]:
   ```

5. 提示指定安装路径，输入回车，选择默认安装路径 `/opt/sequoiadb`；输入路径后回车则表示选择自定义路径

   ```lang-text
   ------------------------------------------------------------
   请指定 SequoiaDBServer 将会被安装到的目录
   安装目录 [/opt/sequoiadb]:
   ```

6. 提示配置 Linux 用户名，输入回车，选择创建默认的用户名（sdbadmin）；输入用户后按回车则表示选择自定义的用户名

   ```lang-text
   ------------------------------------------------------------
   数据库管理用户配置
   配置用于启动 SequoiaDB 的用户名和密码
   用户名[sdbadmin]:
   ```

7. 提示配置 Linux 用户密码，输入回车，选择使用默认密码（sdbadmin）；输入密码后按回车则表示选择自定义密码

   ```lang-text
   密码 [********] :
   确认密码 [********] :
   ```

8. 提示配置服务端口，输入回车，选择默认的服务端口（11790）；输入端口号后按回车则表示选择自定义端口

   ```lang-text
   ------------------------------------------------------------
   集群管理服务端口配置
   配置SequoiaDB集群管理服务端口，集群管理用于远程启动添加和启停数据库节点
   端口 [11790]:
   ```

   >**Note:**  
   >
   > 所有服务器的配置服务端口必须相同。

9. 询问是否允许 SequoiaDB 相关进程开机自启动，输入回车，表示默认设置为开机自启动

   ```lang-text
   ------------------------------------------------------------
   是否允许 SequoiaDB 相关进程开机自启动

   SequoiaDB 相关进程开机自启动 [Y/n]：
   ```

10. 安装完成

   ```lang-text
   正在安装 SequoiaDB Server 于您的电脑中，请稍候。
   安装中
   0% ______________ 50% ______________ 100%
   #########################################
   ------------------------------------------------------------
   安装程序已经完成安装 SequoiaDB Server 于你的电脑中.
   ```

登录SAC
----

安装完成后，OM 会自动启动并开启 8000 端口的 web 服务，用户可以通过浏览器登陆 SAC，并进行集群的部署。假设安装 OM 的机器 IP 为 192.168.1.100，则在浏览器键入  `http://192.168.1.100:8000` 访问 SAC 服务。

输入登录用户名（默认为 admin）和密码（初始密码为 admin）后点击 **登录** 按钮

![登录SAC][login]

   > **Note:**  
   > 第一次使用 SAC 的用户，可以通过[一键部署][deployment_wizard]来完成集群安装。

修改登录密码
----

1. 在 SAC 页面右上方，点击用户名【admin】->【修改密码】
 
  ![修改SAC密码][reset_pwd_1]

2. 输入当前密码（初始密码为 admin）和新密码后点击 **确定** 按钮

    > **Note:**  
    > 用户需牢记新密码，SAC 暂不支持找回密码。

    ![登录SAC][reset_pwd_2]



[^_^]:
    本文使用的所有引用及链接
[download]:http://download.sequoiadb.com/cn/
[deployment_wizard]:manual/SAC/Deployment/deployment_wizard.md
[system_requirement]:manual/Deployment/env_requirement.md
[linux_suggest_setting]:manual/Deployment/linux_suggestion.md

[login]:images/SAC/login.png
[reset_pwd_1]:images/SAC/reset_pwd_1.png
[reset_pwd_2]:images/SAC/reset_pwd_2.png