System 类主要用于获取和操作系统的属性数据，包含的函数如下：

| 名称 | 描述 |
|------|------|
| addAHostMap | 往 host 文件添加一条主机名到 IP 地址的映射关系 |
| addGroup | 添加用户组 |
| addUser | 新增操作系统用户 |
| delAHostMap | 删除 host 文件中的一条指定主机名的映射关系 |
| delGroup | 删除系统用户组 |
| delUser | 删除操作系统用户 |
| getAHostMap | 获取指定主机名在 host 文件中对应的 IP 地址 |
| getCpuInfo | 获取 CPU 的信息 |
| getCurrentUser | 获取当前用户信息 |
| getDiskInfo | 获取磁盘的信息 |
| getEWD | 获取当前 sdb shell 所在的目录 |
| getHostName | 获取主机名 |
| getHostsMap | 获取 host 文件的 IP 与主机名的映射关系 |
| getIpTablesInfo | 获取防火墙信息 |
| getMemInfo | 获取内存信息 |
| getNetcardInfo | 获取网卡的信息 |
| getPID | 获取运行 sdb shell 的进程 ID |
| getProcUlimitConfigs | 获取进程资源限制值|
| getReleaseInfo | 获取操作系统发行版本信息 |
| getSystemConfigs | 获取系统配置信息 |
| getTID | 获取运行 sdb shell 的线程 ID |
| getUserEnv | 获取当前用户的环境变量 |
| isGroupExist | 判断指定用户组是否存在 |
| isProcExist | 判断指定进程是否存在 |
| isUserExist | 判断指定用户是否存在 |
| killProcess | 杀死指定进程 |
| listAllUsers | 列出用户的信息 |
| listGroups | 列出用户组的信息 |
| listLoginUsers | 列出登录用户的信息 |
| listProcess | 列出进程的信息 |
| ping | 判断到达指定主机的网络是否连通 |
| runService | 运行 service 命令 |
| setProcUlimitConfigs | 修改进程资源限制值 |
| setUserConfigs | 修改操作系统用户的配置 |
| snapshotCpuInfo | 获取 CPU 的基本信息 |
| snapshotDiskInfo | 获取磁盘的信息 |
| snapshotMemInfo | 获取内存的基本信息 |
| snapshotNetcardInfo | 获取网卡的详细信息 |
| sniffPort | 判断指定端口是否可用 | 
| type | 获取操作系统类别 |