[^_^]:
    同城双中心部署

本文档主要介绍在同城双中心的部署方案下，如何应对不同级别的灾难。

##灾难应对方案##

###节点故障###

当复制组中超过半数节点发生故障，该复制组将无法提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。如果故障节点未超过半数，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障节点开启运维模式，修复并恢复节点数据即可。

![单节点故障情况][single_breakdown]

###主中心故障###

当主中心发生故障，集群将失去半数以上的节点，导致无法对外提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。

![主中心故障情况][center_breakdown]

<<<<<<< HEAD
###灾备中心故障###
=======
| 子网                   | 主机                  |
|:---------------------- | :--------------------- |
| SUB1    | sdbserver1、sdbserver2 |
| SUB2    | sdbserver3 |
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

当灾备中心发生故障，主中心仍可提供读写服务。针对该情况，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障中心的节点开启运维模式，修复故障中心并恢复节点数据即可。

![灾备中心故障情况][sub2_breakdown]

###数据中心网络故障###

当数据中心发生网络故障，集群仍可提供读写服务。针对该情况，用户仅需修复网络故障并恢复节点数据即可。

![同城网络故障情况][net_breakdown]

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

###恢复故障节点###

1. 逐一开启故障节点的自动全量同步功能

    ```lang-bash
    $ sed -i "s/dataerrorop=.*/dataerrorop=1/g" /opt/sequoiadb/conf/local/<端口号>/sdb.conf
    ```

2. 修复故障

3. 逐一启动故障数据节点

<<<<<<< HEAD
    ```lang-bash
    $ sdbstart -p <故障节点端口号>
    ```
=======
在 SUB1 和 SUB2 两个子网里分别选择 sdbserver1 和 sdbserver3 作为执行分裂和合并操作的机器。

###集群信息初始化（init）

执行集群分裂和合并操作时，需要知道当前自己所在的子网（SUB）所对应的信息，如当前子网里有哪些机器，每台机器上面分别有哪些节点等。正常情况下，这些信息可以通过访问编目复制组（SYSCatalogGroup）来获取，但当灾难导致主中心整体故障的时候，编目复制组已经无法正常工作；因此需要在集群处于正常状态时获取这些信息，以备灾难发生时使用。

1. 切换至安装路径下的 `tools/dr_ha` 目录

   ```lang-bash
   $ cd /opt/sequoiadb/tools/dr_ha
   ```

2. 修改子网 `cluster_opr.js` 文件配置

   SUB1：

   ```lang-javascript
   if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
   if ( typeof(USERNAME) != "string" ) { USERNAME = "sdbadmin" ; }
   if ( typeof(PASSWD) != "string" ) { PASSWD = "sdbadmin" ; }
   if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
   if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1", "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

4. 通过 sdblist 检查各故障数据节点的 GroupID(GID) 和 NodeID(NID) 是否生成，确保故障数据节点注册成功

<<<<<<< HEAD
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
    > db.updateConf({"dataerrorop": 2}, {HostName: "sdbserver3"})
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
=======
   ```lang-javascript
   if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
   if ( typeof(USERNAME) != "string" ) { USERNAME = "sdbadmin" ; }
   if ( typeof(PASSWD) != "string" ) { PASSWD = "sdbadmin" ; }
   if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
   if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1", "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver3:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 2 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false; }
   ```

   > **Note:**
   >
   > 初始化时 ACTIVE 的值决定当前子网的权重，SUB1 中应设置 ACTIVE=true，使主节点分布在主中心内，SUB2 中应设置 ACTIVE=false，防止主节点分布在灾备中心内。



3. 在 sdbserver1 上执行 init

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

   > **Note:**
   >
   > - 执行 `init.sh` 后会生成 `datacenter_init.info` 文件，位于 SequoiaDB 安装目录下，如果此文件已存在，需要先将其删除或备份。
   > - `cluster_opr.js` 中参数 NEEDBROADCASTINITINFO 默认值为 true，表示将初始化的结果文件分发到集群的所有主机上，所以初始化操作在 SUB1 的 sdbserver1 机器上执行即可。


4. 检查集群情况

   sdbserver1：

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       35754     2      5      Y    SYSCoord             2019-01-23-19.30.57  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     36518     1      1      Y    SYSCatalogGroup      2019-01-23-22.27.20  /sequoiadb/cata/11800/
   sequoiadb  11910         data        36517     1002   1006   N    group1               2019-01-23-22.27.20  /sequoiadb/group1/11910/
   sequoiadb  11920         data        36628     1000   1000   Y    group2               2019-01-23-22.30.06  /sequoiadb/group2/11920/
   sequoiadb  11930         data        36648     1001   1003   N    group3               2019-01-23-22.30.21  /sequoiadb/group3/11930/
   Total: 5
   ```

   sdbserver2：

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       12290     2      6      Y    SYSCoord             2019-01-18-07.21.12  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     12305     1      3      N    SYSCatalogGroup      2019-01-18-07.21.12  /sequoiadb/cata/11800/
   sequoiadb  11910         data        12362     1000   1001   N    group1               2019-01-18-07.21.16  /sequoiadb/group1/11910/
   sequoiadb  11920         data        12296     1001   1004   Y    group2               2019-01-18-07.21.12  /sequoiadb/group2/11920/
   sequoiadb  11930         data        12688     1002   1007   Y    group3               2019-01-18-08.55.29  /sequoiadb/group3/11930/
   Total: 5
   ```

   sdbserver3：

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       11626     2      7      Y    SYSCoord             2019-01-20-02.23.30  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     12419     1      4      N    SYSCatalogGroup      2019-01-20-05.01.24  /sequoiadb/cata/11800/
   sequoiadb  11910         data        11704     1000   1002   N    group1               2019-01-20-02.24.11  /sequoiadb/group1/11910/
   sequoiadb  11920         data        11920     1001   1005   N    group2               2019-01-20-02.26.05  /sequoiadb/group2/11920/
   sequoiadb  11930         data        12416     1002   1008   N    group3               2019-01-20-05.01.24  /sequoiadb/group3/11930/
   Total: 5
   ```

  主节点已经全部分布在子网 SUB1 的机器中。


###灾备中心执行分裂（split）

灾难发生时，主中心（SUB1）里的所有机器都不可用，SequoiaDB 集群的三副本中有两副本无法工作。此时需要用“分裂（split）”工具使灾备中心（SUB2）里的一副本脱离原集群，成为具备读写功能的独立集群，以恢复 SequoiaDB 服务。

1. 修改 ACTIVE 参数

   在 sdbserver3 机器上修改 `cluster_opr.js` 中的 ACTIVE 参数为 true
 
   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

2. 执行分裂（split）

   ```lang-bash
   $ sh split.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Kick out host[sdbserver2] from group[SYSCatalogGroup]
   Kick out host[sdbserver1] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Kick out host[sdbserver1] from group[group1]
   Kick out host[sdbserver2] from group[group1]
   Update kicked group[group1] to sdbserver3:11910 succeed
   Kick out host[sdbserver1] from group[group2]
   Kick out host[sdbserver2] from group[group2]
   Update kicked group[group2] to sdbserver3:11920 succeed
   Kick out host[sdbserver1] from group[group3]
   Kick out host[sdbserver2] from group[group3]
   Update kicked group[group3] to sdbserver3:11930 succeed
   Kick out host[sdbserver1] from group[SYSCoord]
   Kick out host[sdbserver2] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver3:11810 succeed
   Update sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly property succeed
   Update all nodes' catalogaddr to sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

3. 检查灾备中心（SUB2）节点状态

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       13590     -      -      Y    SYSCoord             2019-01-20-09.37.52  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     13587     1      4      Y    SYSCatalogGroup      2019-01-20-09.37.52  /sequoiadb/cata/12000/
   sequoiadb  11910         data        13578     1001   1005   Y    group1               2019-01-20-09.37.52  /sequoiadb/group1/11910/
   sequoiadb  11920         data        13581     1002   1008   Y    group2               2019-01-20-09.37.52  /sequoiadb/group2/11920/
   sequoiadb  11930         data        13584     1000   1002   Y    group3               2019-01-20-09.37.52  /sequoiadb/group3/11930/
   Total: 5
   ```

   灾备中心所有节点都是主节点，成为了具备读写功能的单副本 SequoiaDB 集群，可以正常对外提供服务。

![灾备中心执行分裂后][sub2_split]


###主中心故障恢复

主中心（SUB1）的机器从故障中恢复后，有两种可能的情况：

* 主中心（SUB1）中的 SequoiaDB 数据已经遭到严重破坏（比如严重的硬盘故障），SequoiaDB 节点已经无法正常启动，此时需要采取特殊应对措施，如更换硬盘并手工恢复主中心中的数据。
* 主中心（SUB1）中的 SequoiaDB 数据并未遭到破坏，SequoiaDB 节点可以启动并正常工作。

> **Note:**
>
> 主中心（SUB1）的机器恢复正常后，不应手工启动主中心（SUB1）的 SequoiaDB 节点，否则主中心（SUB1）和灾备中心（SUB2）会形成两个独立的可读写 SequoiaDB 集群，如果应用同时连接到 SUB1 和 SUB2，就会出现“脑裂（brain-split）”的情况。

###主中心执行分裂（split）

在执行此步骤前，应满足下面的条件：

灾备中心（SUB2）已经成功执行了分裂操作，灾备中心（SUB2）成为具有读写功能的单副本 SequoiaDB 集群。主中心（SUB1）故障已恢复，且 SequoiaDB 数据没有被损坏。

1. 修改 ACTIVE 参数

   在 sdbserver1 机器上修改 `cluster_opr.js` 中的 ACTIVE 参数为 false

   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
   ```

   设置 ACTIVE=false，使分裂后的2副本集群进入“只读”模式，只有灾备中心的单副本集群具有“写”功能，从而避免了“脑裂（brain-split）”的情况。

2. 开启数据节点自动全量同步

   如果主中心（SUB1）节点是异常终止的，重新启动节点时必须通过全量同步来恢复数据。数据节点参数设置 dataerrorop=2，会阻止全量同步的发生，导致数据节点无法启动。因此，主中心（SUB1）执行分裂操作之前，需要在所有数据节点的配置文件 `sdb.conf` 中设置 dataerrorop=1，才能顺利启动数据节点。

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
   Kick out host[sdbserver3] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver2:11800 succeed
   Kick out host[sdbserver3] from group[group1]
   Update kicked group[group1] to sdbserver2:11800 succeed
   Kick out host[sdbserver3] from group[group2]
   Update kicked group[group2] to sdbserver2:11800 succeed
   Kick out host[sdbserver3] from group[group3]
   Update kicked group[group3] to sdbserver2:11800 succeed
   Kick out host[sdbserver3] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver2:11800 succeed
   Update sdbserver2:11800 catalog's info succeed
   Update sdbserver2:11800 catalog's readonly property succeed
   Stop 11800 succeed in sdbserver1
   Start 11800 by standalone succeed in sdbserver1
   Change sdbserver1:11800 to standalone succeed
   Kick out host[sdbserver3] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver1:11800 succeed
   Kick out host[sdbserver3] from group[group1]
   Update kicked group[group1] to sdbserver1:11800 succeed
   Kick out host[sdbserver3] from group[group2]
   Update kicked group[group2] to sdbserver1:11800 succeed
   Kick out host[sdbserver3] from group[group3]
   Update kicked group[group3] to sdbserver1:11800 succeed
   Kick out host[sdbserver3] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver1:11800 succeed
   Update sdbserver1:11800 catalog's info succeed
   Update sdbserver1:11800 catalog's readonly property  succeed
   Update all nodes' catalogaddr to sdbserver1:11803,sdbserver2:11803 succeed
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver2
   Restart all host nodes succeed
   Done
   ```

4. 检查主中心集群状态

   主中心（SUB1）完成分裂操作后，由三副本集群变成新的两副本“只读”集群，可以分担一部分业务“读”请求。连接主中心集群，执行“写”操作的命令，如创建集合、插入数据、删除数据等，所有“写”操作应该都执行失败，并提示如下错误信息：

   ```lang-bash
   (sdbbp):1 uncaught exception: -287
   This cluster is readonly
   ```

![主中心执行分裂后][sub1_split]


###主中心和灾备中心集群合并（merge）

在执行分裂操作之后，主中心（SUB1）和灾备中心（SUB2）是完全独立的两个集群，灾备中心（SUB2）集群具有“读写”功能，会产生新的业务数据，但新的数据不会同步到主中心（SUB1）中。这种情况下，合并成一个集群后，主节点必须落在灾备中心（SUB2）中。所以执行合并操作前，必须保证主中心（SUB1）设置 ACTIVE=false，灾备中心（SUB2）设置 ACTIVE=true。

1. 设置 ACTIVE

   SUB1：

   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
   ```

   SUB2：

   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

2. 灾备中心（SUB2）先执行合并

   ```lang-bash
   $ sh merge.sh 
   Begin to check args...
   Done
   Begin to check environment...
   Done
   Begin to merge cluster...
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
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

3. 主中心（SUB1）执行合并

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
   Restore group[geoup3] to sdbserver2:11800 succeed
   Restore group[SYSCoord] to sdbserver2:11800 succeed
   Restore sdbserver2:11800 catalog's info succeed
   Update sdbserver2:11800 catalog's readonly property succeed
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
   Update all nodes' catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver2
   Restart all host nodes succeed
   Done
   ```

4. 检查主节点分布情况

   执行合并操作后，确认各复制组的主节点全部分布在灾备中心（SUB2）中

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       15584     2      10     Y    SYSCoord             2019-01-20-12.03.50  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     15581     1      4      Y    SYSCatalogGroup      2019-01-20-12.03.50  /sequoiadb/cata/11800/
   sequoiadb  11910         data        15572     1001   1005   Y    group1               2019-01-20-12.03.50  /sequoiadb/group1/11910/
   sequoiadb  11920         data        15575     1002   1008   Y    group2               2019-01-20-12.03.50  /sequoiadb/group2/11920/
   sequoiadb  11930         data        15578     1000   1002   Y    group3               2019-01-20-12.03.50  /sequoiadb/group3/11930/
   Total: 5
   ```

5. 检查数据同步情况

   执行合并操作后，主中心（SUB1）需要通过数据同步操作追平灾备中心（SUB2）的数据，此过程由 SequoiaDB 自动触发，不需要人工干预。

   可以通过 SequoiaDB 的快照功能检查主中心（SUB1）里的数据节点是否已经完成数据同步并恢复至正常状态。

   sdbserver1：

   ```lang-bash
   $ sdb "db=Sdb('sdbserver1',11810,'sdbadmin','sdbadmin')"
   $ sdb 'db.exec("select * from $SNAPSHOT_DB where NodeName like \"sdbserver1\"")' | grep -E '"NodeName"|Status'
   ```

   输出结果如下：
   ```lang-json
     "NodeName": "sdbserver1:11800",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver1:11810",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver1:11910",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver1:11920",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver1:11930",
     "ServiceStatus": true,
     "Status": "Normal",
   ```

   sdbserver2：

   ```lang-bash
   $ sdb "db=Sdb('sdbserver2',11810,'sdbadmin','sdbadmin')"
   $ sdb 'db.exec("select * from $SNAPSHOT_DB where NodeName like \"sdbserver2\"")' | grep -E '"NodeName"|Status'
   ```

   输出结果如下：
   ```lang-json
     "NodeName": "sdbserver2:11800",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver2:11810",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver2:11910",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver2:11920",
     "ServiceStatus": true,
     "Status": "Normal",
     "NodeName": "sdbserver2:11930",
     "ServiceStatus": true,
     "Status": "Normal",
   ```

   由上面的输出可以看到，在合并操作完成一段时间之后，主中心（SUB1）里所有的节点都已经完成数据同步。

6. 关闭数据节点自动全量同步

   当合并操作完成并且主中心（SUB1）和灾备中心（SUB2）的数据追平，后续不再需要数据节点的自动全量同步，因此需要将所有数据节点的 dataerrorop 参数改回最初的设置，即 dataerrorop=2。

   连接协调节点，动态刷新节点配置参数

   ```lang-bash
   $ sdb "db=Sdb('sdbserver1',11810,'sdbadmin','sdbadmin')"
   $ sdb "db.updateConf({dataerrorop:2}, {GroupName:'group1'})"
   $ sdb "db.updateConf({dataerrorop:2}, {GroupName:'group2'})"
   $ sdb "db.updateConf({dataerrorop:2}, {GroupName:'group3'})"
   $ sdb "db.reloadConf()"
   ```

7. 再次执行初始化（init），恢复集群最初状态

   由于合并之后，集群中主节点全部分布在灾备集群（SUB2）中，因此需要再次执行初始化操作，将主节点重新分布到主中心（SUB1）中。

   > **Note:**
   >
   > 再次执行初始化操作之前，需要先删除 SequoiaDB 安装目录下的 `datacenter_init.info` 文件，否则执行 `init.sh` 会提示如下错误：
   >
   > Already init. If you want to re-init, you should to remove the file: /opt/sequoiadb/datacenter_init.info



8. 主中心（SUB1）设置 ACTIVE=true

   ```lang-bash
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true; }
   ```

9. 灾备中心（SUB2）设置 ACTIVE=false

   ```lang-bash
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false; }
   ```

10. 主中心（SUB1）执行初始化

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

   > **Note:**
   >
   > `cluster_opr.js` 中参数 NEEDBROADCASTINITINFO 默认值为 true，表示将初始化的结果文件分发到集群的所有主机上，所以初始化操作在 SUB1 的 sdbserver1 机器上执行即可。

11. 检查主节点分布情况

   重新初始化之后，确认各复制组的主节点全部分布在主中心（SUB1）中

   sdbserver1：

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       40898     2      8      Y    SYSCoord             2019-01-24-05.35.42  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     41150     1      1      N    SYSCatalogGroup      2019-01-24-05.37.29  /sequoiadb/cata/11800/
   sequoiadb  11910         data        40886     1001   1003   N    group1               2019-01-24-05.35.42  /sequoiadb/group1/11910/
   sequoiadb  11920         data        40889     1002   1006   N    group2               2019-01-24-05.35.42  /sequoiadb/group2/11920/
   sequoiadb  11930         data        40892     1000   1000   N    group3               2019-01-24-05.35.42  /sequoiadb/group3/11930/
   Total: 5
   ```

   sdbserver2：

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       15961     2      9      Y    SYSCoord             2019-01-18-16.03.39  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     16208     1      3      Y    SYSCatalogGroup      2019-01-18-16.05.46  /sequoiadb/cata/11800/
   sequoiadb  11910         data        15949     1001   1004   Y    group1               2019-01-18-16.03.39  /sequoiadb/group1/11910/
   sequoiadb  11920         data        15952     1002   1007   Y    group2               2019-01-18-16.03.39  /sequoiadb/group2/11920/
   sequoiadb  11930         data        15955     1000   1001   Y    group3               2019-01-18-16.03.40  /sequoiadb/group3/11930/
   Total: 5
   ```
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2


<<<<<<< HEAD
=======
   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11810         coord       15584     2      10     Y    SYSCoord             2019-01-20-12.03.50  /sequoiadb/coord/11810/
   sequoiadb  11800         catalog     15581     1      4      N    SYSCatalogGroup      2019-01-20-12.03.50  /sequoiadb/cata/11800/
   sequoiadb  11910         data        15572     1001   1005   N    group1               2019-01-20-12.03.50  /sequoiadb/group1/11910/
   sequoiadb  11920         data        15575     1002   1008   N    group2               2019-01-20-12.03.50  /sequoiadb/group2/11920/
   sequoiadb  11930         data        15578     1000   1002   N    group3               2019-01-20-12.03.50  /sequoiadb/group3/11930/
   Total: 5
   ```



  
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

[^_^]:
    本文使用到的所有链接
[twodatacenter_cluster]:images/Distributed_Engine/Maintainance/HA_DR/twodatacenter_cluster.png
[single_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/single_breakdown.png
[sub2_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/sub2_breakdown.png
[net_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/net_breakdown.png
[center_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/center_breakdown.png
[recovery]:manual/Distributed_Engine/Maintainance/HA_DR/twodatacenter.md#灾难恢复
[replication]:manual/Distributed_Engine/Architecture/Replication/architecture.md
[location_principle]:manual/Distributed_Engine/Architecture/Location/location_principle.md#位置亲和性
[startMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startMaintenanceMode.md