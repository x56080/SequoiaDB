

`quickDeploy.sh` 是 SDB Schedule 的快速部署脚本，支持创建调度服务节点（createnode）、 创建数据源站点（createsite）

## 脚本配置文件说明

quickDeploy.conf 是脚本的唯一配置入口

配置示例：
```txt
# 安装用户
install_user="sdbadmin"

# rootsite 连接信息
rootsite_url="192.168.17.183:11810,192.168.31.25:11810"
rootsite_user="sdbadmin"
rootsite_password="sdbadmin1"

# 调度节点端口（多个用逗号分割）
node_list="9001,9002"

# 数据源名称（多个用逗号分割）
datasource_list="datasource1,datasource2"

# 工具使用的集合空间名，默认是 SDB_SCHEDULE_SYSTEM
system_cs_name="SDB_SCHEDULE_SYSTEM"
```

## 使用方式

```shell
用法:
  ./quickDeploy.sh <action>

action:
  deploy       创建节点、创建数据源站点
  createnode   只创建调度节点
  createsite   只创建数据源站点
  help|-h      显示帮助
```

## 使用示例

1. 编辑配置文件 quickDeploy.conf，配置 rootsite 连接信息、要部署的节点端口等信息

2. 完整部署，会创建节点和数据源站点

```bash
# ./quickDeploy.sh deploy
```

3. 若只需创建调度节点

```bash
# ./quickDeploy.sh createnode
```

4. 若只需创建数据源站点

```bash
# ./quickDeploy.sh createsite
```
