在使用 SequoiaDB 过程，可能会出现扩容数据库的需求。这时，用户需要往一个现有的集群中，添加新的主机，并把新的节点部署到这些新的主机上。

在添加新的主机过程，用户需要注意一下几点：

1. 在新的主机上安装与其他主机相同的操作系统。
2. 在新的主机上配置好主机名，并配置好主机名与IP地址的映射关系。
3. 修改所有主机的/etc/hosts文件，使得所有主机可以两两通过主机名访问对方（例如：使用主机名ssh到其它主机）。
4. 按照[Linux系统要求](installation/system/system_requirement.md)和[Linux推荐配置](installation/system/linux_suggest_settings.md)要求配置系统的内核参数。
5. 按照 [数据库安装](installation/deployment/command_installation/installation.md)一节，在新的机器上安装 SequoiaDB 的 run 包。安装时，注意配置管理服务端口与现有系统的端口保持一致（默认为11790）。
