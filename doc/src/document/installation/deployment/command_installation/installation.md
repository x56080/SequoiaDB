本文档将介绍如何通过命令行安装的方式将 SequoiaDB 巨杉数据库安装到本地主机。

### 下载 SequoiaDB 安装包

请到 SequoiaDB 巨杉数据库的官方网站下载相应版本的安装包。

下载地址：[SequoiaDB 巨杉数据库](http://download.sequoiadb.com/cn/)

### 安装 SequoiaDB 巨杉数据库步骤

下述安装过程，使用名称为`sequoiadb-3.2-linux_x86_64-installer.tar.gz`的 SequoiaDB 产品包为示例。

> **Note:**
>
> - 确保系统满足硬件和软件要求
> - 安装过程需要使用 root 用户权限
> - 如果需要图形界面模式安装，请确保 X Server 服务已启动
> - 请确保所有主机都设置了主机名，并且都设置了主机名/IP地址映射关系
> - 请确保所有主机两两之间可通过主机名建立网络连接（如 ssh 主机名）

1. 参照[Linux环境推荐配置] (installation/system/linux_suggest_settings)调整 Linux 系统的环境配置

2. 以root 用户登陆目标主机，解压 SequoiaDB 巨杉数据库产品包，并为解压得到的 `sequoiadb-3.2-linux_x86_64-installer.run` 安装包赋可执行权限

   ```shell
   # tar -zxvf sequoiadb-3.2-linux_x86_64-installer.tar.gz
   # chmod u+x sequoiadb-3.2-linux_x86_64-installer.run
   ```

3. 使用 root 用户运行 `sequoiadb-3.2-linux_x86_64-installer.run` 包

   ```shell
   # ./sequoiadb-3.2-linux_x86_64-installer.run --mode text --SMS false
   ```

4. 提示选择向导语言，可根据需要输入 1 选择英文，或者输入 2 选择中文

   ```shell
   Language Selection
   Please select the installation language
   [1] English - English
   [2] Simplified Chinese - 简体中文
   Please choose an option [1] :2
   ```

5. 显示安装协议，输入 1 表示忽略阅读并同意协议，输入 2 表示读取完整协议内容

   ```shell
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

6. 指定 SequoiaDB 安装路径，输入完毕后按回车。若没有输入直接回车，将使用默认的安装路径（/opt/sequoiadb）

   ```shell
   ------------------------------------------------------------
   请指定 SequoiaDB Server 将会被安装到的目录
   安装目录 [/opt/sequoiadb]:
   ```

7. 询问是否强制安装，y 表示强制安装，安装时发现有相关进程存在则会尝试停止进程，N 表示非强制安装，安装时发现有相关进程存在，就会报错退出。默认为非强制安装

   ```shell
   ------------------------------------------------------------
   是否强制安装？强制安装时可能会强杀残留进程
   是否强制安装 [y/N]:
   ```

8. 提示配置 Linux 用户名和用户组，输入完毕后按回车。若没有输入直接回车，将会创建默认的用户名（sdbadmin）和用户组（sdbadmin_group）。该用户名用于运行 SequoiaDB 服务

   ```shell
   ------------------------------------------------------------
   数据库管理用户配置
   配置用于启动 SequoiaDB 的用户名、用户组和密码
   用户名 [sdbadmin]:
   用户组 [sdbadmin_group]:
   ```

9. 提示配置刚才创建的 Linux 用户的密码，输入完毕后按回车。若没有输入直接回车，将会使用默认密码（sdbadmin）

   ```shell
   密码 [********] :
   确认密码 [********] :
   ```

10. 提示配置服务端口，输入完毕后按回车。若没有输入直接回车，将使用默认的服务端口号（11790）

    ```shell
    ------------------------------------------------------------
    集群管理服务端口配置
    配置 SequoiaDB 集群管理服务端口，集群管理用于远程启动添加和启停数据库节点
    端口 [11790]:
    ```

11. 询问是否允许 SequoiaDB 巨杉数据库相关进程开机自启动，Y 表示允许，n 表示不允许。默认为允许

    ```shell
    ------------------------------------------------------------
    是否允许 SequoiaDB 相关进程开机自启动
    Sequoiadb相关进程开机自启动 [Y/n]:
    ```

12. 询问是否继续安装，Y 表示继续，n 表示不继续。默认为继续

    ```shell
    ------------------------------------------------------------
    设定现在已经准备将 SequoiaDB Server 安装到您的电脑.
    您确定要继续? [Y/n]:
    ```

13. 当屏幕上显示以下信息，表示 SequoiaDB 已经安装完成

    ```shell
    正在安装 SequoiaDB Server 于您的电脑中，请稍候。
    安装中
    0% ______________ 50% ______________ 100%
    #########################################
    ------------------------------------------------------------
    安装程序已经完成安装 SequoiaDB Server 于你的电脑中.
    ```

14. 使用如下命令查看 SequoiaDB 的安装信息。其中 SDBADMIN_USER 表示 SequoiaDB 相关进程所属用户的用户名，INSTALL_DIR 表示 SequoiaDB 的安装目录。

    ```
    # cat /etc/default/sequoiadb
    NAME=sdbcm
    SDBADMIN_USER=sdbadmin
    INSTALL_DIR=/opt/sequoiadb
    ```
15. 切换到 SDBADMIN_USER 指定的用户。

    ```
    # su - sdbadmin
    ```
16. 进入 SequoiaDB 安装目录，使用如下命令进行安装检查，如能正常查到 SequoiaDB 的版本信息，说明 SequoiaDB 安装成功。

    ```
    $ cd /opt/sequoiadb
    $ ./bin/sequoiadb  --version
    SequoiaDB shell version: 3.2
    Release: 37126
    2018-10-14-13.15.29
    ```

当所有的主机都安装了 SequoiaDB 后，用户可以根据自身的情况(可参考[规划数据库部署](installation/deployment/command_installation/planning_database_deployment.md))，选择部署[单机模式](installation/deployment/command_installation/standalone.md)或者[集群模式](installation/deployment/command_installation/cluster.md)的环境。只有成功部署了环境，用户才能使用 SequoiaDB 进行数据操作。

### 安装包参数描述

| 参数         | 描述                                                       | 默认值   |
| -------------- | ---------------------------------------------------------- | ------------------- |
| version | 安装包版本 | 无                   |
| unattendedmodeui | 显示不同级别的用户交互，none 表示不交互，minimal 表示不需要用户交互只显示安装进度，minimalWithDialogs 表示除了显示安装进度外，还会根据安装程序逻辑弹出窗口与用户交互 | 不交互                  |
| optionfile | 配置文件，run 包可以通过指定配置文件给安装包进行传参 | 无                   |
| installer-language | 安装过程中的提示语言类型，支持英文和中文， en 表示英文， zh_CN 表示中文                               | en| 
| mode      | 安装模式，包含静默安装、文本模式安装以及图形界面安装，text 表示文本模式安装， unattended 表示静默安装，gtk 表示 Linux 和 Linux x64上的常规IB默认执行模式，要求Gtk库在系统中存在，安装程序呈现Gtk外观，xwindow 表示 Linux/Uniux系统上的轻量级图形执行模式                                  | 图形界面安装 |
| prefix | 安装路径                       | /opt/sequoiadb |
| force     | 是否强制安装                                 | false              |
| username     | 安装目录用户                                 | sdbadmin              |
| groupname     | 安装目录用户组                                 | sdbadmin_group              |
| userpasswd     | 安装用户密码                                 | sdbadmin              |
| port     | SequoiaDB 集群管理端口                                 | 11790              |
| processAutoStart     | 机器重启时是否自动重启 SequoiaDB 相关进程，true 表示是，false 表示否                                 | true              |
| SMS     | 是否安装 OM 服务，true 表示是，false 表示否                                 | false              |
| om-port     | SequoiaDB OM 服务端口号                                | 11780             |
| http-port    | SequoiaDB SAC 网页端口号                                | 8000             |