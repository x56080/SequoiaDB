[^_^]:
    数据库引擎安装
    作者：陈思琴
    时间：
    评审意见
    王涛：
    许建辉：时间：
    市场部：时间：20190704


目前，SequoiaDB 巨杉数据库有命令行安装和[可视化安装][sac]两种安装方式。本文档主要介绍通过命令行方式在本地主机安装 SequoiaDB 巨杉数据库。

> **Note:**
>
> 如果有多台主机，则每台主机都需要重复下述安装步骤将 SequoiaDB 安装至本地。

下载安装包
----

用户可前往 SequoiaDB 官方网站下载相应版本的[安装包][download_sequoiadb]。

安装
----

### 安装前准备

- 安装过程需要使用 root 用户权限
- 如果需要图形界面安装，应确保 X Server 服务处于运行状态
- 需要参照[操作系统配置][os_setup]和 [Linux 环境推荐配置][linux_suggestion]调整 Linux 系统配置

### 安装步骤

以下安装过程将使用名称为 `sequoiadb-{version}-linux_x86_64-installer.run` 的产品包为示例。

> **Note:**
>
> 用户在安装过程中若输入有误，可按 ctrl+退格键进行删除。

1. 以root 用户登陆目标主机，解压 SequoiaDB 巨杉数据库产品包，并为解压得到的 `sequoiadb-{version}-linux_x86_64-installer.run` 安装包赋可执行权限

   ```lang-bash
   # tar -zxvf sequoiadb-{version}-linux_x86_64-installer.tar.gz
   # chmod u+x sequoiadb-{version}-linux_x86_64-installer.run
   ```

2. 使用 root 用户运行 `sequoiadb-{version}-linux_x86_64-installer.run` 包

    ```lang-bash
    # ./sequoiadb-{version}-linux_x86_64-installer.run --mode text
    ```

   >**Note:**
   >
   > 执行安装包时不添加参数 --mode，则进入图形界面安装模式 

3. 提示选择向导语言，输入2，选择中文

    ```lang-text
    Language Selection
    
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] : 2
    ```

4. 显示安装协议，输入回车，忽略阅读并同意协议；输入2后按回车则表示读取全部文件
 
    ```lang-text
    ----------------------------------------------------------------------------
    由BitRock InstallBuilder评估本所建立
    
    欢迎来到 SequoiaDB Server 安装程序
    
    ----------------------------------------------------------------------------
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
    请选择一个选项 [1] : 
    ```

5. 提示指定安装路径，输入回车，选择默认路径 `/opt/sequoiadb`；输入路径后按回车则表示选择自定义路径

    ```lang-text
    ----------------------------------------------------------------------------
    请指定 SequoiaDB Server 将会被安装到的目录
    
    安装目录 [/opt/sequoiadb]: 
    ```

6. 提示是否强制安装，输入回车，选择不强制安装；输入y后按回车则表示选择强制安装

    ```lang-text
    ----------------------------------------------------------------------------
    是否强制安装？强制安装时可能会强杀残留进程
    
    是否强制安装 [y/N]: 
    ```

7. 提示配置 Linux 用户名和用户组，该用户名用于运行 SequoiaDB 服务，输入回车，选择创建默认的用户名（sdbadmin）和用户组（sdbadmin_group）；输入用户名和用户组后按回车则表示选择自定义的用户名和用户组
 
    ```lang-text
    ----------------------------------------------------------------------------
    数据库管理用户配置
    
    配置用于启动SequoiaDB的用户名、用户组和密码
    
    用户名 [sdbadmin]: 
   
    用户组 [sdbadmin_group]: 
    ```

8. 提示配置刚才创建的 Linux 用户密码，输入回车，选择使用默认密码（sdbadmin ）；输入密码后按回车则表示选择自定义密码

    ```lang-text
    密码 [********] :
    确认密码 [********] :
    ```

9. 提示配置服务端口，输入回车，选择使用默认的服务端口号（11790）；输入端口号后按回车则表示选择自定义端口

    ```lang-text
    ----------------------------------------------------------------------------
    集群管理服务端口配置
    
    配置SequoiaDB集群管理服务端口,集群管理用于远程启动添加和启停数据库节点
   
    集群管理服务端口 [11790]: 
    ```

    > **Note:**
    >
    > 用户需要在多台主机上安装部署 SequoiaDB 时，所有主机配置的服务端口必须设置为相同的值。

10. 提示选择允许 SequoiaDB 相关进程开机自启动，输入回车，表示设置为开机自启动

     ```lang-text
     ----------------------------------------------------------------------------
     是否允许Sequoiadb相关进程开机自启动？
     
     Sequoiadb相关进程开机自启动 [Y/n]: 
     ```

11. 提示是否安装 OM 服务，输入回车，表示不安装；如果选择其他选项则输入选项后按回车即可

     ```lang-text
     ----------------------------------------------------------------------------
     是否安装OM服务
     
     [1] true
     [2] false
     [3] only
     请选择一个选项 [2] : 
     ```
 
     > **Note:**
     >
     > - true 表示安装 OM，且重启已存在的 SequoiaDB 集群；
     > - only 表示只安装 OM，不会重启已存在的 SequoiaBD 集群。

12. 输入回车，确认继续

     ```lang-text
     ----------------------------------------------------------------------------
     设定现在已经准备将 SequoiaDB Server 安装到您的电脑.
     
     您确定要继续? [Y/n]: 
     ```

13. 安装完成

     ```lang-text
     ----------------------------------------------------------------------------
     正在安装 SequoiaDB Server 于您的电脑中，请稍候.
     
      安装中
      0% ______________ 50% ______________ 100%
      #########################################
     
     ----------------------------------------------------------------------------
     安装程序已经完成安装 SequoiaDB Server 于你的电脑中.
     ```

当所有的主机都安装了 SequoiaDB 后，用户可参考[规划数据库部署][readme]并结合自身情况，选择部署单机模式或者集群模式的环境。只有成功部署了环境，才能使用 SequoiaDB 进行数据操作。


[^_^]:
     本文使用的所有引用及链接
[download_sequoiadb]:http://download.sequoiadb.com/cn/index-cat_id-1 
[install_requirement]:manual/Deployment/env_requirement.md
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[catalog_node]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[coord_node]:manual/Distributed_Engine/Architecture/Node/coord_node.md
[os_setup]:manual/Deployment/env_requirement.md
[linux_suggestion]:manual/Deployment/linux_suggestion.md
[sac]:manual/SAC/install_login.md
[readme]:manual/Deployment/Readme.md#规划数据库部署
