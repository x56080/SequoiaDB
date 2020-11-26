[^_^]:
    同城三中心部署
    作者：杨磊
    时间：20190531
    评审意见
    王涛：
    许建辉：
    市场部：

本文档将介绍如何采用三副本机制部署 SequoiaDB 巨杉数据库同城三中心灾备集群，“同城三中心”即一个主数据中心和两个同城灾备中心。这种模式下能够实现“多活”，多中心之间地位均等，正常模式下协同工作，并行的为业务访问提供服务，实现了对资源的充分利用，避免一个或两个备份中心处于闲置状态，造成资源与投资浪费。另外，如果一个数据中心发生故障或灾难，其它数据中心可以正常运行并对关键业务或全部业务实现接管。

同城三中心灾备架构
----

同城三中心灾备架构如图，每个中心部署一个副本：
![同城三中心集群部署][threedatacenter]:

因为同城灾备网络的带宽有限，需要严格控制 SequoiaDB 集群对同城带宽的占用，阻止数据节点在异常终止后进行自动全量同步。设置 sdbcm 节点参数 AutoStart=FALSE 和 EnableWatch=FALSE，设置每个数据节点参数 dataerrorop=2。在数据同步方面，应采用 SequoiaDB 提供的[节点强一致性][consistency]功能，当数据写入主节点时，数据库会确保节点间的数据都同步完成后才返回，这样即使在主机房发生整体灾难时也能保证数据的完整性与安全性。

灾难应对方案
----

###单节点故障

由于采用了三副本高可用架构，个别节点故障情况下，数据组依然可以正常工作。针对个别节点的故障场景，无需采取特别的应对措施，只需要及时修复故障节点，并通过自动数据同步或者人工数据同步的方式去恢复故障节点数据即可。
![单节点故障情况][1c3d_singlenode_down]

###单个数据中心整体故障

由于每个中心部署单副本，所以单个数据中心故障等同于单节点故障。针对单个数据中心整体故障的场景，无需采取特别的应对措施，只需要及时修复故障中心节点，并通过自动数据同步或者人工数据同步的方式去恢复故障节点数据即可。

![单个数据中心故障情况][1c3d_singlecenter_down]

###同城网络故障

当主中心与其中一个灾备中心网络故障无法进行通信时，由于采用了三副本架构，应用程序可以访问剩下两副本集群，所以无需采取特别的应对措施，只需要及时修复网络故障，修复后通过自动数据同步或者人工数据同步的方式去恢复灾备节点的数据即可。当主中心与两个灾备中心网络故障无法进行通信时，整个集群环境将会失去超过 1/2 的节点，如果从每个数据组来看，相当于每个数据组有两个数据节点出现了故障，存活的节点只剩余一个。这种情况下就需要用到“分裂（split）”和“合并（merge）”工具做一些特殊处理，把主中心分裂成独立的单副本集群，对外提供读写服务。

###两个中心整体故障

当其中两个数据中心整体发生故障，整个集群环境将会失去超过 1/2 的节点。这种情况下就需要用到“分裂（split）”和“合并（merge）”工具做一些特殊处理，把存活的中心分裂成单独的集群提供读写服务。

灾难恢复
----
SequoiaDB 对于容灾处理提供了[容灾切换合并工具][split_merge]。本章节以主中心和灾备中心 B 整体故障为例，在只剩下灾备中心 A 单副本的情况下，如何进行灾难恢复。

###集群信息初始化（init）

执行集群分裂和合并操作时，需要知道当前自己所在的子网（SUB）所对应的信息，如当前子网里有哪些机器，每台机器上面分别有哪些节点等。正常情况下，这些信息可以通过访问编目复制组（SYSCatalogGroup）来获取，但当灾难导致灾备中心整体故障的时候，编目复制组已经无法正常工作；因此需要在集群处于正常状态时获取这些信息，以备灾难发生时使用。

>  **Note:**
>
>  初始化操作需要在灾难发生前且集群处于正常状态下执行。


为了使复制组内主节点分布到主中心，将子网划分如下：

| 子网 | 主机                   |
| :--- | :--------------------- |
| SUB1 | sdbserver1             |
| SUB2 | sdbserver2、sdbserver3 |

1. 修改 `cluster_opr.js` 文件配置

   ```lang-javascript
   if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
   if ( typeof(USERNAME) != "string" ) { USERNAME = "sdbadmin" ; }
   if ( typeof(PASSWD) != "string" ) { PASSWD = "sdbadmin" ; }
   if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
   if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver2", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

   > **Note:**
   >
   > 初始化时 ACTIVE 的值决定当前子网节点的权重，SUB1 中应设置 ACTIVE=true，使主节点分布在主中心内。

2. 在 sdbserver1 上执行 init

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh init.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to init cluster...
   Start to copy init file to cluster host
   Copy init file to sdbserver3 success
   Copy init file to sdbserver2 success
   Done
   Begin to update catalog and data nodes's config...Done
   Begin to reload catalog and data nodes's config...Done
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

1. 在 sdbserver2 上修改 `cluster_opr.js` 文件配置

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver2:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

2. 执行分裂（split）

   ```lang-bash
   [sdbadmin@sdbserver2 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver2
   Start 11800 by standalone succeed in sdbserver2
   Change sdbserver2:11800 to standalone succeed
   Kick host[sdbserver1] from group[SYSCatalogGroup]
   Kick host[sdbserver3] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver2:11800 succeed
   Kick host[sdbserver1] from group[group1]
   Kick host[sdbserver3] from group[group1]
   Update kicked group[group1] to sdbserver2:11800 succeed
   Kick host[sdbserver1] from group[group2]
   Kick host[sdbserver3] from group[group2]
   Update kicked group[group2] to sdbserver2:11800 succeed
   Kick host[sdbserver1] from group[group3]
   Kick host[sdbserver3] from group[group3]
   Update kicked group[group3] to sdbserver2:11800 succeed
   Kick host[sdbserver1] from group[SYSCoord]
   Kick host[sdbserver3] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver2:11800 succeed
   Update sdbserver2:11800 catalog's info succeed
   Update sdbserver2:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver2:11803 succeed
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
   [sdbadmin@sdbserver1 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver1
   Start 11800 by standalone succeed in sdbserver1
   Change sdbserver1:11800 to standalone succeed
   Kick host[sdbserver2] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver1:11800 succeed
   Kick host[sdbserver2] from group[group1]
   Update kicked group[group1] to sdbserver1:11800 succeed
   Kick host[sdbserver2] from group[group2]
   Update kicked group[group2] to sdbserver1:11800 succeed
   Kick host[sdbserver2] from group[group3]
   Update kicked group[group3] to sdbserver1:11800 succeed
   Kick host[sdbserver2] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver1:11800 succeed
   Update sdbserver1:11800 catalog's info succeed
   Update sdbserver1:11800 catalog's readonly prop succeed
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Kick host[sdbserver2] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Kick host[sdbserver2] from group[group1]
   Update kicked group[group1] to sdbserver3:11800 succeed
   Kick host[sdbserver2] from group[group2]
   Update kicked group[group2] to sdbserver3:11800 succeed
   Kick host[sdbserver2] from group[group3]
   Update kicked group[group3] to sdbserver3:11800 succeed
   Kick host[sdbserver2] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver3:11800 succeed
   Update sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver1:11803,sdbserver3:11803 succeed
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
   [sdbadmin@sdbserver2 dr_ha]$ sh merge.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
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
   Update sdbserver2:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver2
   Restart all host nodes succeed
   Done
   ```

3. SUB2 执行合并

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh merge.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
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
   Update sdbserver1:11800 catalog's readonly prop succeed
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Restore group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Restore group[group1] to sdbserver3:11800 succeed
   Restore group[group2] to sdbserver3:11800 succeed
   Restore group[group3] to sdbserver3:11800 succeed
   Restore group[SYSCoord] to sdbserver3:11800 succeed
   Restore sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803 succeed
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
   [sdbadmin@sdbserver1 dr_ha]$ sh init.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to init cluster...
   Start to copy init file to cluster host
   Copy init file to sdbserver2 success
   Copy init file to sdbserver3 success
   Done
   Begin to update catalog and data nodes's config...Done
   Begin to reload catalog and data nodes's config...Done
   Begin to reelect all groups...Done
   Done
   ```





[^_^]:
    本文使用到的所有链接

[threedatacenter]:images/Maintainance/HA_DR/threedatacenter.png
[1c3d_singlenode_down]:images/Maintainance/HA_DR/1c3d_singlenode_down.png
[1c3d_singlecenter_down]:images/Maintainance/HA_DR/1c3d_singlecenter_down.png
[split_merge]:manual/Maintainance/Mgmt_Tools/split_merge.md
[consistency]:manual/infrastructure/Replication/primary_secondary_consistency.md