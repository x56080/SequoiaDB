[^_^]:
    同城三中心部署

本文档主要介绍在同城三中心的部署方案下，如何应对不同级别的灾难。

##灾难应对方案##

###节点故障###

当复制组中超过半数节点发生故障，该复制组将无法提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。如果故障节点未超过半数，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障节点开启运维模式，修复并恢复节点数据即可。

![节点故障情况][1c3d_singlenode_down]

###单个数据中心故障###

当单个数据中心发生故障，集群仍对外提供读写服务。针对该情况，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障中心的节点开启运维模式，修复故障中心并恢复节点数据即可。

![单个数据中心故障情况][1c3d_singlecenter_down]

###两个数据中心故障###

当两个数据中心发生故障，集群将失去半数以上的节点，导致无法对外提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。

![两个中心整体故障][1c3d_dualcenter_down]

###数据中心网络故障###

当一个数据中心发生网络故障，另外两个数据中心仍可提供读写服务。针对该情况，用户仅需修复网络故障并恢复节点数据即可。

![同城网络故障情况][1c3d_network]

当两个数据中心发生网络故障，集群将失去半数以上的节点，导致无法对外提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。

![同城网络故障情况][1c3d_network_down]

##灾难恢复##

在进行灾难恢复时，用户需根据实际情况选取待恢复的主机，并在该主机上执行后续恢复步骤，实现业务的接管。待恢复主机的选取规则如下：

- 在主中心未故障的情况下，优先选取主中心的机器作为待恢复主机。
- 在主中心故障且存在多个可用灾备中心的情况下，优先选取与主中心具有[亲和性][location_principle]的灾备中心。该灾备中心的任意机器均可作为待恢复主机。

下述以 SequoiaDB 安装目录 `/opt/sequoiadb/`、编目节点 11800、协调节点 11810、集群鉴权用户名“sdbadmin”和用户密码“sdbadmin”为例，介绍灾难恢复步骤。

###恢复编目复制组###

如果集群因数据中心整体故障而导致无法对外提供服务，用户需恢复编目复制组。数据节点故障和网络故障场景可跳过此步骤。

1. 关闭鉴权功能

    ```lang-bash
    $ echo "auth=false" >> /opt/sequoiadb/conf/local/11800/sdb.conf
    ```

2. 重启编目节点，使配置生效

    ```lang-bash
    $ sdbstop -p 11800
    $ sdbstart -p 11800
    ```

3. 通过 SDB Shell 将当前主机的编目节点升主

    ```lang-javascript
    > var cata = new Sdb("localhost", 11800)
    > cata.forceStepUp()
    ```

4. 在编目复制组中开启 Critical 模式

    ```lang-javascript
    > var db = new Sdb("localhost", 11810)
    > var cataRG = db.getRG("SYSCatalogGroup")
    > cataRG.startCriticalMode({Location: "Guangzhou.Panyu", MinKeepTime: 100, MaxKeepTime: 1000})
    ```

###恢复数据复制组###

1. 在发生故障的数据复制组中开启 Critical 模式

    ```lang-javascript
    > var dataRG = db.getRG("group1")
    > dataRG.startCriticalMode({Location: "Guangzhou.Panyu", MinKeepTime: 100, MaxKeepTime: 1000})
    ```

    > **Note:**
    >
    > 参数 MaxKeepTime 表示 Critical 模式的最高运行窗口时间。如果在该时间内故障未修复，系统将强制解除 Critical 模式，集群将回到不可用状态。因此用户需根据实际的故障修复耗时，指定该参数的取值，避免多次执行开启操作。

2. 查看是否成功开启

    ```lang-javascript
    > db.list(SDB_LIST_GROUPMODES)
    ```

<<<<<<< HEAD
    输出结果如下，字段 GroupMode 显示为 critical 表示开启成功：

    ```lang-json
    {
      "_id": {
        "$oid": "6458b62bdfc87b1c4344e16b"
      },
      "GroupID": 1,
      "GroupMode": "critical",
      "Properties": [
        {
          "Location": " Guangzhou.Panyu",
          "MinKeepTime": "2023-05-08-18.23.23.445185",
          "MaxKeepTime": "2023-05-09-09.23.23.445185",
          "UpdateTime": "2023-05-08-16.43.23.445185"
        }
      ]
    }
    ···
    ```
=======
   ```lang-bash
   $ sh init.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to init cluster...
   Begin to copy init file to cluster hosts
   Copy init file to sdbserver3 succeed
   Copy init file to sdbserver2 succeed
   Done
   Begin to update catalog and data nodes' config...Done
   Begin to reload catalog and data nodes' config...Done
   Begin to reelect all groups...Done
   Done
   ```

   > **Note:**
   >
   > - 执行 `init.sh` 后会生成 `datacenter_init.info` 文件，位于 SequoiaDB 安装目录下，如果此文件已存在，需要先将其删除或备份。
   > - `cluster_opr.js` 中参数 NEEDBROADCASTINITINFO 默认值为 true，表示将初始化的结果文件分发到集群的所有主机上，所以初始化操作在 SUB1 的“sdbserver1”机器上执行即可。

###灾备中心 A 执行分裂（split）

主中心和灾备中心 B 整体故障时，SequoiaDB 集群的三副本中有两副本无法工作。此时需要用分裂工具使灾备中心 A 里的单副本脱离原集群，成为具备读写功能的独立集群，以恢复 SequoiaDB 服务。

此时子网划分如下：

| 子网 | 主机                   |
| :--- | :--------------------- |
| SUB1 | sdbserver2             |
| SUB2 | sdbserver1、sdbserver3 |

1. 切换至安装路径下的 `tools/dr_ha` 目录

   ```lang-bash
   $ cd /opt/sequoiadb/tools/dr_ha
   ```

2. 在 sdbserver2 上修改 `cluster_opr.js` 文件配置

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver2:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

3. 执行分裂（split）

   ```lang-bash
   $ sh split.sh
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver2
   Start 11800 by standalone succeed in sdbserver2
   Change sdbserver2:11800 to standalone succeed
   Kick out host[sdbserver1] from group[SYSCatalogGroup]
   Kick out host[sdbserver3] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver2:11800 succeed
   Kick out host[sdbserver1] from group[group1]
   Kick out host[sdbserver3] from group[group1]
   Update kicked group[group1] to sdbserver2:11800 succeed
   Kick out host[sdbserver1] from group[group2]
   Kick out host[sdbserver3] from group[group2]
   Update kicked group[group2] to sdbserver2:11800 succeed
   Kick out host[sdbserver1] from group[group3]
   Kick out host[sdbserver3] from group[group3]
   Update kicked group[group3] to sdbserver2:11800 succeed
   Kick out host[sdbserver1] from group[SYSCoord]
   Kick out host[sdbserver3] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver2:11800 succeed
   Update sdbserver2:11800 catalog's info succeed
   Update sdbserver2:11800 catalog's readonly property succeed
   Update all nodes' catalogaddr to sdbserver2:11803 succeed
   Restart all nodes succeed in sdbserver2
   Restart all host nodes succeed
   Done
   ```

   此时灾备中心 A（sdbserver2）组成了具备读写功能的单副本独立集群，可以正常对外提供服务。


###主中心和灾备中心 B 故障恢复

主中心和灾备中心 B 中的机器从故障中恢复后，有两种可能的情况：

* SequoiaDB 数据已经遭到严重破坏（比如严重的硬盘故障），节点已经无法正常启动，此时需要采取特殊应对措施，如更换硬盘并手工恢复主中心中的数据。
* SequoiaDB 数据并未遭到破坏，节点可以启动并正常工作。

机器恢复正常后，不应手工启动 SUB2 的 SequoiaDB 节点，否则 SUB1（灾备中心A） 和 SUB2（主中心、灾备中心B） 会形成两个独立的可读写 SequoiaDB 集群，如果应用同时连接到 SUB1 和 SUB2，就会出现“脑裂（brain-split）”的情况。


###主中心和灾备中心 B 执行分裂（split）

在执行此步骤前，应满足下面的条件：

灾备中心 A（SUB1）已经成功执行了分裂操作，成为具有读写功能的 SequoiaDB 集群。SUB2 节点故障已恢复，且 SequoiaDB 数据没有被损坏。

1. 在 sdbserver1 修改 `cluster_opr.js` 文件配置

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 2 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
   ```

   设置 ACTIVE=false，使分裂后的两副本集群进入“只读”模式，只有灾备中心 A 的单副本集群具有“写”功能，从而避免了“脑裂（brain-split）”的情况。

2. 开启数据节点自动全量同步

 如果 SUB2 中节点是异常终止的，重新启动节点时必须通过全量同步来恢复数据。数据节点参数设置 dataerrorop=2，会阻止全量同步的发生，导致数据节点无法启动。因此，执行分裂操作之前，需要在所有数据节点的配置文件 `sdb.conf` 中设置 dataerrorop=1，才能顺利启动数据节点。

3. 执行分裂

   ```lang-bash
   $ sh split.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver1
   Start 11800 by standalone succeed in sdbserver1
   Change sdbserver1:11800 to standalone succeed
   Kick out host[sdbserver2] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver1:11800 succeed
   Kick out host[sdbserver2] from group[group1]
   Update kicked group[group1] to sdbserver1:11800 succeed
   Kick out host[sdbserver2] from group[group2]
   Update kicked group[group2] to sdbserver1:11800 succeed
   Kick out host[sdbserver2] from group[group3]
   Update kicked group[group3] to sdbserver1:11800 succeed
   Kick out host[sdbserver2] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver1:11800 succeed
   Update sdbserver1:11800 catalog's info succeed
   Update sdbserver1:11800 catalog's readonly property succeed
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Kick out host[sdbserver2] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Kick out host[sdbserver2] from group[group1]
   Update kicked group[group1] to sdbserver3:11800 succeed
   Kick out host[sdbserver2] from group[group2]
   Update kicked group[group2] to sdbserver3:11800 succeed
   Kick out host[sdbserver2] from group[group3]
   Update kicked group[group3] to sdbserver3:11800 succeed
   Kick out host[sdbserver2] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver3:11800 succeed
   Update sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly property succeed
   Update all nodes' catalogaddr to sdbserver1:11803,sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

###集群合并（merge）

在执行“分裂（split）”操作之后，SUB1 和 SUB2 是完全独立的两个集群，SUB1 集群具有“读写”功能，会产生新的业务数据，但新的数据不会同步到 SUB2 中。这种情况下，合并成一个集群后，主节点必须落在 SUB1 中。所以执行“合并（merge）”操作前，必须保证 SUB1 设置 ACTIVE=true，SUB2 设置 ACTIVE=false。

1. 设置 ACTIVE

   SUB1：

   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

   SUB2:
 
   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }-
   ```

2. 灾备中心A（SUB1） 先执行合并

   ```lang-bash
   $ sh merge.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to merge cluster...
   Stop 11800 succeed in sdbserver2
   Start 11800 by standalone succeed in sdbserver2
   Change sdbserver2:11800 to standalone succeed
   Restore group[SYSCatalogGroup] to sdbserver2:11800 succeed
   Restore group[group1] to sdbserver2:11800 succeed
   Restore group[group2] to sdbserver2:11800 succeed
   Restore group[group3] to sdbserver2:11800 succeed
   Restore group[SYSCoord] to sdbserver2:11800 succeed
   Restore sdbserver2:11800 catalog's info succeed
   Update sdbserver2:11800 catalog's readonly property succeed
   Update all nodes' catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver2
   Restart all host nodes succeed
   Done
   ```

3. SUB2 执行合并

   ```lang-bash
   $ sh merge.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to merge cluster...
   Stop 11800 succeed in sdbserver1
   Start 11800 by standalone succeed in sdbserver1
   Change sdbserver1:11800 to standalone succeed
   Restore group[SYSCatalogGroup] to sdbserver1:11800 succeed
   Restore group[group1] to sdbserver1:11800 succeed
   Restore group[group2] to sdbserver1:11800 succeed
   Restore group[group3] to sdbserver1:11800 succeed
   Restore group[SYSCoord] to sdbserver1:11800 succeed
   Restore sdbserver1:11800 catalog's info succeed
   Update sdbserver1:11800 catalog's readonly property succeed
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Restore group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Restore group[group1] to sdbserver3:11800 succeed
   Restore group[group2] to sdbserver3:11800 succeed
   Restore group[group3] to sdbserver3:11800 succeed
   Restore group[SYSCoord] to sdbserver3:11800 succeed
   Restore sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly property succeed
   Update all nodes' catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

4. 关闭数据节点自动全量同步

当合并操作完成并且 SUB2 和 SUB1 的数据追平，后续不再需要数据节点的自动全量同步，因此需要将所有数据节点的 dataerrorop 参数改回最初的设置，即 dataerrorop=2。

   连接协调节点，动态刷新节点配置参数

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sdb "db=Sdb('sdbserver1',11810,'sdbadmin','sdbadmin')"
   [sdbadmin@sdbserver1 dr_ha]$ sdb "db.updateConf({dataerrorop:2}, {GroupName:'group1'})"
   [sdbadmin@sdbserver1 dr_ha]$ sdb "db.updateConf({dataerrorop:2}, {GroupName:'group2'})"
   [sdbadmin@sdbserver1 dr_ha]$ sdb "db.updateConf({dataerrorop:2}, {GroupName:'group3'})"
   [sdbadmin@sdbserver1 dr_ha]$ sdb "db.reloadConf()"
   ```

###再次执行初始化（init）

集群合并之后，需要再次执行初始化操作，将主节点重新分布到主中心，恢复集群最初状态。

> **Note:**
>
> 再次执行初始化操作之前，需要先删除 SequoiaDB 安装目录下的 `datacenter_init.info` 文件，否则执行 `init.sh` 会提示如下错误：
>
> Already init. If you want to re-init, you should to remove the file: /opt/sequoiadb/datacenter_init.info


此时的子网划分如下：

| 子网 | 主机                   |
| :--- | :--------------------- |
| SUB1 | sdbserver1             |
| SUB2 | sdbserver2、sdbserver3 |

1. 修改 `cluster_opr.js` 文件配置

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver2", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

2. 在 sdbserver1 上执行 init

   ```lang-bash
   $ sh init.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to init cluster...
   Begin to copy init file to cluster hosts
   Copy init file to sdbserver2 succeed
   Copy init file to sdbserver3 succeed
   Done
   Begin to update catalog and data nodes' config...Done
   Begin to reload catalog and data nodes' config...Done
   Begin to reelect all groups...Done
   Done
   ```
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

###恢复故障节点###

1. 逐一开启故障节点的自动全量同步功能

    ```lang-bash
    $ sed -i "s/dataerrorop=.*/dataerrorop=1/g" /opt/sequoiadb/conf/local/<端口号>/sdb.conf
    ```

2. 修复故障

3. 逐一启动故障数据节点

    ```lang-bash
    $ sdbstart -p <故障节点端口号>
    ```

4. 通过 sdblist 检查各故障数据节点的 GroupID(GID) 和 NodeID(NID) 是否生成，确保故障数据节点注册成功

    ```lang-bash
    $ sdblist -l
    ```

5. 逐一启动故障编目节点

    ```lang-bash
    $ sdbstart -p <故障节点端口号>
    ```

6. 检查各主机的节点是否成功启动

    ```lang-bash
    $ sdblist -l
    ```

7. 检查节点健康检测快照，确定各节点状态恢复为 Normal

###恢复集群配置###

1. 通过 SDB Shell 检查 Critical 模式是否解除

    ```lang-javascript
    > db.list(SDB_LIST_GROUPMODES)
    ```

    如果字段 GroupMode 显示为 critical 表示未解除，需执行如下命令手动解除：

    ```lang-javascript
    > var dataRG = db.getRG("group1")
    > dataRG.stopCriticalMode()
    > var cataRG = db.getRG("SYSCatalogGroup")
    > cataRG.stopCriticalMode()
    ```

    > **Note:**
    >
    > 手动解除 Critical 模式前请确保集群已恢复，否则集群将回到不可用状态。

2. 重新选举各复制组中的主节点，使主节点恢复至故障前所在的数据中心

    ```lang-javascript
    > dataRG.reelect({Seconds: 60})
    > cataRG.reelect({Seconds: 60})
    ```

3. 关闭非 ActiveLocation 位置集下，所有节点的自动全量同步功能

    ```lang-javascript
    > db.updateConf({"dataerrorop": 2}, {HostName: ["sdbserver2", "sdbserver3"]})
    ```

4. 通过命令行检查鉴权功能的状态

    ```lang-bash
    $ cat /opt/sequoiadb/conf/local/11800/sdb.conf
    ```

    如果参数 auth 的取值为 false ，表示鉴权功能为关闭状态，用户需手动执行如下命令开启鉴权：

    ```lang-bash
    $ sed -i "s/auth=.*/auth=true/g" /opt/sequoiadb/conf/local/11800/sdb.conf
    ```

5. 重启编目节点，使配置生效

    ```lang-bash
    $ sdbstop -p 11800
    $ sdbstart -p 11800
    ```

[^_^]:
    本文使用到的所有链接
[threedatacenter]:images/Distributed_Engine/Maintainance/HA_DR/threedatacenter.png
[1c3d_singlenode_down]:images/Distributed_Engine/Maintainance/HA_DR/1c3d_singlenode_down.png
[1c3d_singlecenter_down]:images/Distributed_Engine/Maintainance/HA_DR/1c3d_singlecenter_down.png
[split_merge]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md
[consistency]:manual/Distributed_Engine/Architecture/Replication/primary_secondary_consistency.md
[1c3d_network]:images/Distributed_Engine/Maintainance/HA_DR/1c3d_network.png
[1c3d_network_down]:images/Distributed_Engine/Maintainance/HA_DR/1c3d_network_down.png
[1c3d_dualcenter_down]:images/Distributed_Engine/Maintainance/HA_DR/1c3d_dualcenter_down.png
[recovery]:manual/Distributed_Engine/Maintainance/HA_DR/threedatacenter.md#灾难恢复
[location_principle]:manual/Distributed_Engine/Architecture/Location/location_principle.md#位置亲和性
[startMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startMaintenanceMode.md