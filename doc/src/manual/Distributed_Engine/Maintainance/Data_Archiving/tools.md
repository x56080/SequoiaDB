## 调度服务管理工具

### 概述

schadmin.sh 提供调度服务配置管理功能，包含 createnode、deletenode、createsite

- createnode：创建调度服务节点

- deletenode：删除调度服务节点

- createsite：创建站点

### createnode

createnode 子命令提供创建调度服务节点的功能。创建节点的同时，元数据服务 SequoiaDB 将做为 rootsite 站点被创建。

#### 参数选项

| 选项       | 缩写 | 描述                                           | 是否必填 |
|----------| ---- |----------------------------------------------|------|
| --port   | -p   | 节点端口号                                        | 是    |
| --ms-url |      | 元数据服务 SequoiaDB 协调节点地址，如：sdb1:1180,sdb2:1180 | 是    |
| --ms-user   |      | 元数据服务 SequoiaDB 用户名                          | 是    |
| --ms-passwd |      | 元数据服务 SequoiaDB 用户密码                         | 是    |
| --systemCsName |      | 指定系统表的集合空间名，默认 'SDB_SCHEDULE_SYSTEM'         | 否    |
| --systemCsStoreDomain |      | 指定系统表的集合空间存储的数据域                             | 否    |

#### 示例

```bash
$ schadmin.sh createnode -p 9000 --ms-url sdb1:11810 --ms-user sdbadmin --ms-passwd sequoiadb
```

### deletenode

deletenode 子命令提供删除调度服务节点的功能.

#### 参数选项

| 选项       | 缩写  | 描述                                             | 是否必填 |
|----------|-----|------------------------------------------------|------|
| --port   | -p  | 节点端口号                                          | 是    |
| --ms-url |      | 元数据服务 SequoiaDB 协调节点地址，如：sdb1:1180,sdb2:1180 | 否    |
| --ms-user   |      | 元数据服务 SequoiaDB 用户名                          | 否    |
| --ms-passwd |      | 元数据服务 SequoiaDB 用户密码                         | 否    |
| --systemCsName |      | 指定系统表的集合空间名，默认 'SDB_SCHEDULE_SYSTEM'         | 否    |

- 当本地机器存在调度服务节点时，则可以不填写 --ms-url、--ms-user、--ms-passwd，工具会从已有节点的配置文件中读取。

#### 示例

```bash
$ schadmin.sh deletenode -p 9000 --ms-url sdb1:11810 --ms-user sdbadmin --ms-passwd sequoiadb
```

### createsite

createsite 子命令提供创建数据源站点的功能。

#### 参数选项

| 选项          | 缩写  | 描述                                           | 是否必填 |
|-------------|-----|----------------------------------------------|------|
| --name      |     | 站点名                                          | 是    |
| --ms-url |      | 元数据服务 SequoiaDB 协调节点地址，如：sdb1:1180,sdb2:1180 | 否    |
| --ms-user   |      | 元数据服务 SequoiaDB 用户名                          | 否    |
| --ms-passwd |      | 元数据服务 SequoiaDB 用户密码                         | 否    |
| --systemCsName |      | 指定系统表的集合空间名，默认 'SDB_SCHEDULE_SYSTEM'         | 否    |
| --ds-name   |     | 站点绑定的数据源名字，该数据源需要在 rootsite 的数据源列表中          | 是    |

#### 示例

- 创建数据源集群，并确保该集群在 rootsite 的数据源列表中；假设数据源名为 `datasource`

    ```bash
    $ schadmin.sh createsite --name dsSite --ms-url server1:11810 --ms-user sdbadmin --ms-passwd sdbadmin --ds-name datasource
    ```

## 调度服务节点管理工具

### 概述

schctl.sh 工具提供调度服务节点管理相关的功能，包含 start、stop、list

- start：启动节点

- stop：停止节点

- list：列取节点

### start

start 子命令提供调度服务节点的启动功能。

#### 参数选项

| 选项      | 缩写 | 描述                                                         | 是否必填 |
| --------- | ---- | ------------------------------------------------------------ | -------- |
| --port    | -p   | 指定特定端口，启动该节点                                     | 否       |
| --type    | -t   | 启动指定类型的节点，可选值：schedule-server、all             | 否       |
| --timeout |      | 指定超时时间，在规定时间内节点未正常运行，判定启动失败，单位：秒，默认值：50s | 否       |

#### 示例

1. 启动端口号为 8180 的调度服务节点

   ```bash
   # schctl.sh start -p 8180
   ```

2. 启动所有节点

   ```bash
   # schctl.sh start -t all
   ```

### stop

stop 子命令提供调度服务节点的停止功能

#### 参数选项

| 选项    | 缩写 | 描述                                        | 是否必填 |
| ------- | ---- | ------------------------------------------- | -------- |
| --port  | -p   | 指定特定端口，停止该节点                    | 否       |
| --type  | -t   | 指定节点类型，可选值：schedule-server、 all | 否       |
| --force | -f   | 强制停止节点                                | 否       |

#### 示例

1. 停止端口号为 8180 的调度服务节点

   ```bash
   $ schctl.sh stop -p 8180
   ```

2. 停止所有节点

   ```bash
   $ schctl.sh stop -t all
   ```

3. 强制停止所有节点

   ```bash
   $ schctl.sh stop -t all -f
   ```

### list

list 子命令提供查询和显示本机调度服务节点的功能

#### 参数选项

| 选项     | 缩写  | 描述                                       | 是否必填 |
|--------|-----|------------------------------------------|------|
| --port | -p  | 端口号，默认查询所有端口号                            | 否    |
| --mode | -m  | 查看本地所有节点:'local'，查看运行中的节点:'run',默认:'run' | 否    |

#### 示例

1. 查看本机运行中调度服务的节点

   ```bash
   $ schctl.sh list
   ```

2. 查看本机所有调度服务的节点

   ```bash
   $ schctl.sh list -m local
   ```

3. 查看特定端口调度服务的节点

   ```bash
   $ schctl.sh list -p 8180 -m local
   ```

   