Oma 类主要用于集群管理，包含的函数如下：

| 名称 | 描述 |
|------|------|
| Oma() | 集群管理对象 |
| addAOmaSvcName() | 将目标机器为 sdbcm 设置的服务端口号写到该 sdbcm 的配置文件中 |
| delAOmaSvcName() | 将目标机器 sdbcm 的服务端口号从其配置文件中删除 |
| close() | 关闭 Oma 连接对象 |
| createCoord() | 在目标集群控制器（sdbcm）所在的机器中创建一个 coord 节点 |
| createData()| 在目标集群控制器（sdbcm）所在的机器中创建一个 standalone 节点 |
| createOM() | 在目标集群控制器（sdbcm）所在的机器中创建 sdbom 服务进程（ SequoiaDB 管理中心进程） |
| getAOmaSvcName() | 获取目标机器 sdbcm 的服务端口 |
| getIniConfigs() | 获取 INI 文件的配置信息 |
| getNodeConfigs() | 从配置文件中获取指定端口的数据库节点的配置信息 |
| getOmaConfigFile() | 获取 sdbcm 的配置文件 |
| getOmaConfigs() | 获取 sdbcm 的配置信息 |
| getOmaInstallFile() | 获取安装信息文件 |
| getOmaInstallInfo() | 从安装信息文件中获取安装信息 |
| listNodes() | 列出当前所连 sdbcm 所在机器符合条件的所有节点的信息 |
| reloadConfigs() | sdbcm 重新加载其配置文件的内容，并使其生效 |
| removeCoord() | 在目标集群控制器（sdbcm）所在的机器中删除一个 coord 节点 |
| removeData() | 在目标集群控制器（sdbcm）所在的机器中删除指定的 standalone 节点 |
| removeOM() | 在目标集群控制器（sdbcm）所在的机器中删除 sdbom 服务进程（SequoiaDB 管理中心进程） |
| setIniConfigs() | 把配置信息写入 INI 文件 |
| setNodeConfigs() | 对指定端口的数据库节点，用新的节点配置信息覆盖该节点原来配置文件上的配置信息 |
| setOmaConfigs() | 把 sdbcm 的配置信息写入到其配置文件 |
| start() | 启动 sdbcm 服务 |
| startAllNodes() | 在目标集群控制器（sdbcm）所在的机器中启动所有属于指定业务的节点 |
| stopAllNodes() | 在目标集群控制器（sdbcm）所在的机器中停止所有属于指定业务的节点 |
| startNode() | 在目标集群控制器（sdbcm）所在的机器中启动指定节点 |
| stopNode() | 在目标集群控制器（sdbcm）所在的机器中停止指定节点 |
| startNodes() | 通过服务端口启动节点 |
| stopNodes() | 在目标集群控制器（sdbcm）所在的机器中停止指定节点 |
| updateNodeConfigs() | 对指定端口的数据库节点，用新的节点配置信息更新该节点原来配置文件上的配置信息 |