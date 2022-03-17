[^_^]:
    离线升级


离线升级指在升级过程中服务会暂时性不可用，在整个升级过程完成之后恢复服务。本文档将说明在一台主机上的离线升级流程，按照相同步骤，用户可以依次完成所有主机上的软件升级。

下载安装包
----

用户可前往 SequoiaDB 巨杉数据库官网下载相应版本的[安装包][download]。

##升级##

###升级说明###

* 升级 SequoiaDB 需要使用操作系统 root 用户权限，用户需确认已经获取了对应权限。 
* 升级过程中输入的参数不接受非英文字符。

###升级步骤###

本文档以从 2.8.7 企业版升级到 3.2 企业版为例进行说明，其它版本间的升级与之基本一致。升级前需要先将新版软件安装包上传到目标主机，并确保软件包具有可执行权限。

1. 运行安装包，并加上升级参数 --upgrade

   ```lang-bash
   $ ./sequoiadb-3.2-linux_x86_64-enterprise-installer.run --upgrade true
   ```

   > **Note:**
   >
   > 如果在 XShell 中执行安装包，可能导致弹出图形界面。此时可添加参数--mode text，重新运行以上升级命令。

2. 提示选择向导语言，输入2，选择中文
 
   ```lang-text
    Language Selection
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] :2
    ```

3. 显示安装协议，输入回车，表示忽略阅读并同意协议；输入2，表示读取完整协议内容

    ```lang-text   
    显示安装协议，如果需要读取全部文件，输入2。输入1表示忽略阅读并同意协议。
     ……
    [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
    [2] 查看详细的协议内容
    请选择选项 [1] :
   ```

4. 提示切换到升级模式，输入回车，选择升级模式
 
    ```lang-text   
    是否切换到升级模式[upgrade/cover]？
    [1] upgrade
    [2] cover
    请选择一个选项 [1] :
   ```

   > **Note:**  
   >
   > 参数 installmode 指定为 cover 时，会进行覆盖安装，即强制覆盖当前版本，无论版本是否兼容。

5. 升级完成，可通过 `sequoiadb --version` 检查版本号，并通过 `sdblist` 检查节点是否均已正常启动

   ```lang-text
    正在安装 SequoiaDB Server 于您的电脑中，请稍候。
    安装中
    0% ______________ 50% ______________ 100%
    开始升级 ......
    **************************  检查列表 *************************************
    检查：系统配置文件/etc/default/sequoiadb存在 ...... ok
    检查：在/etc/default/sequoiadb中获取安装路径和用户名 ...... ok
    检查：安装目录/opt/sequoiadb不为空 ...... ok
    检查：旧版本 2.8.7 Enterprise 与新版本 3.2 Enterprise 兼容 ...... ok
    检查：磁盘空间足够 ...... ok
    检查：主机名存在，主机名能映射到本机ip地址 ...... ok
    检查：umask配置 ...... ok
    检查：用户sdbadmin存在，并获取用户组 ...... ok
    检查：相关进程已停止 ...... ok
    #########################################
    ------------------------------------------------------------
    安装程序已经完成安装 SequoiaDB Server 于你的电脑中.
   ```

6. 在集群中所有主机完成软件升级后，如果 SequoiaDB 是由 v3.6/5.0.3 以下版本升级至 v3.6/5.0.3 及以上版本，需要手动执行 [sdbupgradeidx][upgrade_index] 工具进行索引升级。




[^_^]:
    本文中用到的所有链接
[download]:http://download.sequoiadb.com/cn/
[compatibility]:manual/Maintainance/Upgrade/compatibility.md
[upgrade_index]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/upgrade_index.md
[report]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/upgrade_index.md#示例