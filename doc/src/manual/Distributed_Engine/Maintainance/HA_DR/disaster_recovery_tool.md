[^_^]:
    容灾工具的使用
    作者：杨磊
    时间：20190930
    评审意见
    王涛：
    许建辉：
    市场部：20191121

SequoiaDB 巨杉数据库对于容灾处理提供了 split/merge 工具。split 工具的基本工作原理是使某些副本从原有集群中分裂出来，成为一个新的集群，单独提供读写服务；其余副本成为一个新的集群，仅提供读服务。merge 工具的基本工作原理是将分裂出去的副本重新合并到原有的集群中，恢复到集群最初的状态。关于工具的更多信息可参考[容灾切换合并工具][split_merge]章节。 

同城双中心
----

### 灾备环境初始化

在[同城双中心架构][twodatacenter]下，将机器划分为两个子网 SUB1（sdbserver1、sdbserver2）和 SUB2（sdbserver3），并在 sdbserver1 和 sdbserver3 上进行配置的修改。

1. 修改 sdbserver1 的 `cluster_opr.js` 文件

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

2. 修改 sdbserver3 的 `cluster_opr.js` 文件

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

3. 在 sdbserver1 执行 init

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

   > **Note:**
   >
   > - 执行 `init.sh` 后会生成 `datacenter_init.info` 文件，位于 SequoiaDB 安装目录下，如果此文件已存在，需要先将其删除或备份。
   > - `cluster_opr.js` 中参数 NEEDBROADCASTINITINFO 默认值为 true，表示将初始化的结果文件分发到集群的所有主机上，所以初始化操作在 SUB1 的“sdbserver1”机器上执行即可。

### 灾备切换

当主中心的所有机器发生故障时，SUB1 里的所有机器都不可用，SequoiaDB 集群的三副本中有两副本无法工作。此时需要用分裂（split）工具使灾备中心（SUB2）里的一副本脱离原集群，成为具备读写功能的独立集群，以恢复 SequoiaDB 服务。

1. 修改 sdbserver3 的 `cluster_opr.js` 文件

   ```lang-javascript
   /* 是否激活该子网集群，取值 true/false */
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```
2.  在 sdbserver3 执行分裂（split）

   ```lang-bash
   [sdbadmin@sdbserver3 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Kick host[sdbserver2] from group[SYSCatalogGroup]
   Kick host[sdbserver1] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Kick host[sdbserver1] from group[group1]
   Kick host[sdbserver2] from group[group1]
   Update kicked group[group1] to sdbserver3:11910 succeed
   Kick host[sdbserver1] from group[group2]
   Kick host[sdbserver2] from group[group2]
   Update kicked group[group2] to sdbserver3:11920 succeed
   Kick host[sdbserver1] from group[group3]
   Kick host[sdbserver2] from group[group3]
   Update kicked group[group3] to sdbserver3:11930 succeed
   Kick host[sdbserver1] from group[SYSCoord]
   Kick host[sdbserver2] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver3:11810 succeed
   Update sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

   此时灾备中心已完成切换，成为独立的业务集群，并且可以正常对外提供服务。

3. 主中心故障恢复后，在 sdbserver1 修改 `cluster_opr.js` 中的配置项

   ```lang-javascript
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
   ```

   设置 ACTIVE=false，使分裂后的2副本集群进入“只读”模式，只有灾备中心的单副本集群具有“写”功能，从而避免了脑裂（brain-split）的情况。

4. 开启数据节点自动全量同步

   如果主中心（SUB1）节点是异常终止的，重新启动节点时必须通过全量同步来恢复数据。数据节点参数设置 dataerrorop=2，会阻止全量同步的发生，导致数据节点无法启动。因此，主中心（SUB1）执行“分裂（split）”操作之前，需要在所有数据节点的配置文件 `sdb.conf` 中设置 dataerrorop=1，才能顺利启动数据节点。

5. 在 sdbserver1 执行分裂（split）

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   ...
   Done
   ```

### 灾难修复

1. 执行合并（merge）

  当主中心的所有故障都完成修复，需要将两个中心已分离的独立集群进行合并，恢复最初的状态。用户可以在 sdbserver1 和 sdbserver3 同时执行命令。

   ```lang-bash
   $ sh merge.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
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
   Update sdbserver3:11800 catalog's readonly prop succeed
   ...
   Update all nodes's catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803 succeed
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

2. 关闭数据节点自动全量同步

   当合并操作完成后，并且主中心（SUB1）和灾备中心（SUB2）的数据追平，后续不再需要数据节点的自动全量同步，因此需要将所有数据节点的 dataerrorop 参数改回最初的设置，即 dataerrorop=2。

3. 再次执行初始化（init），恢复集群最初状态

   由于合并之后，集群中主节点全部分布在灾备集群（SUB2）中，因此需要再次执行初始化操作，将主节点重新分布到主中心（SUB1）中。

   修改 sdbserver1 的 `cluster_opr.js` 文件

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ grep 'ACTIVE =' cluster_opr.js
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true; }
   ```

   修改 sdbserver3 的 `cluster_opr.js` 文件

   ```lang-bash
   [sdbadmin@sdbserver3 dr_ha]$ grep 'ACTIVE =' cluster_opr.js
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false; }
   ```

   在 sdbserver1 执行初始化

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

   > **Note:**
   >
   > 再次执行初始化操作之前，需要先删除 SequoiaDB 安装目录下的 `datacenter_init.info` 文件，否则执行 `init.sh` 会提示如下错误：
   >
   > Already init. If you want to re-init, you should to remove the file: /opt/sequoiadb/datacenter_init.info


同城三中心
----

### 灾备环境初始化

在[同城三中心架构][threedatacenter]下，初始化时将机器划分为两个子网 SUB1（sdbserver1）和 SUB2（sdbserver2、 sdbserver3）。

1. 修改 sdbserver1 的 `cluster_opr.js` 文件

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
   > 初始化时 ACTIVE 的值决定当前子网的权重，SUB1 中应设置 ACTIVE=true，使主节点分布在主中心内，SUB2 中应设置 ACTIVE=false，防止主节点分布在灾备中心内。

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

### 灾备切换

当主中心和灾备中心 B 的所有机器发生故障时，SequoiaDB 集群的三副本中有两副本无法工作。此时需要用分裂工具使灾备中心A里的单副本脱离原集群，成为具备读写功能的独立集群，以恢复 SequoiaDB 服务。

此时子网划分如下：

| 子网 | 主机                   |
| :--- | :--------------------- |
| SUB1 | sdbserver2             |
| SUB2 | sdbserver1、sdbserver3 |

1. 修改 sdbserver2 的 `cluster_opr.js` 中的配置项

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver2:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```
2. 在 sdbserver2 执行分裂（split）

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

   此时灾备中心 A（sdbserver2）已完成切换，成为独立的业务集群，并且可以正常对外提供服务。

3. 主中心和灾备中心 B 故障恢复后，在 sdbserver1 修改 `cluster_opr.js` 中的配置项

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver2" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 2 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
   ```

   设置 ACTIVE=false，使分裂后的二副本集群进入“只读”模式，只有灾备中心 A 的单副本集群具有“写”功能，从而避免了脑裂（brain-split）的情况。

4. 开启数据节点自动全量同步

   如果 SUB2 中节点是异常终止的，重新启动节点时必须通过全量同步来恢复数据。数据节点参数设置 dataerrorop=2，会阻止全量同步的发生，导致数据节点无法启动。因此，主中心（SUB1）执行分裂操作之前，需要在所有数据节点的配置文件 `sdb.conf` 中设置 dataerrorop=1，才能顺利启动数据节点。

5. 在 sdbserver1执行分裂

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver1
   Start 11800 by standalone succeed in sdbserver1
   ...
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

### 灾难修复

当主中心和灾备中心 B 的所有故障都完成修复，需要将三个中心已分离的独立集群进行合并，恢复最初的状态。

1. 在 sdbserver2 执行合并

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

2. 在 sdbserver1 执行合并

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

3. 关闭数据节点自动全量同步

   当合并（merge）操作完成后，并且 SUB1 和 SUB2 的数据追平，后续不再需要数据节点的自动全量同步，因此需要将所有数据节点的 dataerrorop 参数改回最初的设置，即 dataerrorop=2。

4. 再次执行初始化(init)，恢复集群最初状态

   集群合并之后，需要再次执行初始化操作，将主节点重新分布到主中心，恢复集群最初状态。
此时的子网划分如下：

   | 子网 | 主机                   |
   | :--- | :--------------------- |
   | SUB1 | sdbserver1             |
   | SUB2 | sdbserver2、sdbserver3 |

   在 sdbserver1 修改 `cluster_opr.js` 中的配置项

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver2", "sdbserver3" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

   在 sdbserver1 上执行初始化

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

   > **Note:**
   >
   > 再次执行初始化操作之前，需要先删除 SequoiaDB 安装目录下的 `datacenter_init.info` 文件，否则执行 `init.sh` 会提示如下错误：
   >
   > Already init. If you want to re-init, you should to remove the file: /opt/sequoiadb/datacenter_init.info



两地三中心
----

[两地三中心][twocity_threedatacenter]架构下容灾工具的使用，可以参考[同城双中心][twodatacenter_usage]。



三地五中心
----

### 灾备环境初始化
在[三地五中心架构][threecity_fivedatacenter]下，初始化时将机器划分为两个子网 SUB1（sdbserver1）和 SUB2（sdbserver2、sdbserver3、sdbserver4、sdbserver5)。

1. 修改 sdbserver1 的 `cluster_opr.js` 文件

   ```lang-javascript
   if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
   if ( typeof(USERNAME) != "string" ) { USERNAME = "sdbadmin" ; }
   if ( typeof(PASSWD) != "string" ) { PASSWD = "sdbadmin" ; }
   if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
   if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver2", "sdbserver3", "sdbserver4", "sdbserver5" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

   > **Note:**
   >
   > 初始化时 ACTIVE 的值决定当前子网的权重，SUB1 中应设置 ACTIVE=true，使主节点分布在主中心内，SUB2 中应设置 ACTIVE=false，防止主节点分布在灾备中心内。

2. 在 sdbserver1 执行 init

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh init.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to init cluster...
   Start to copy init file to cluster host
   Copy init file to sdbserver3 success
   Copy init file to sdbserver4 success
   Copy init file to sdbserver5 success
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
   > - `cluster_opr.js` 中参数 NEEDBROADCASTINITINFO 默认值为 true，表示将初始化的结果文件分发到集群的所有主机上，所以初始化操作在 SUB1 的 sdbserver1 机器上执行即可。

### 灾备切换

当城市1和城市2整体故障时，SUB1 里的所有机器都不可用，SequoiaDB 集群的五副本中有三副本无法工作。此时需要用分裂（split）工具使城市2里的两副本脱离原集群，成为具备读写功能的独立集群，以恢复 SequoiaDB 服务。

此时子网划分如下：

| 子网 | 主机                               |
| :--- | :--------------------------------- |
| SUB1 | sdbserver3、sdbserver4             |
| SUB2 | sdbserver1、sdbserver2、sdbserver5 |

1. 修改 sdbserver4 的 `cluster_opr.js` 文件

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver3", "sdbserver4" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver2", "sdbserver5" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver4:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

2. 在 sdbserver4 执行分裂

   ```lang-bash
   [sdbadmin@sdbserver4 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver3
   Start 11800 by standalone succeed in sdbserver3
   Change sdbserver3:11800 to standalone succeed
   Kick host[sdbserver1] from group[SYSCatalogGroup]
   Kick host[sdbserver2] from group[SYSCatalogGroup]
   Kick host[sdbserver5] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver3:11800 succeed
   Kick host[sdbserver1] from group[group1]
   Kick host[sdbserver2] from group[group1]
   Kick host[sdbserver5] from group[group1]
   Update kicked group[group1] to sdbserver3:11800 succeed
   Kick host[sdbserver1] from group[group2]
   Kick host[sdbserver2] from group[group2]
   Kick host[sdbserver5] from group[group2]
   Update kicked group[group2] to sdbserver3:11800 succeed
   Kick host[sdbserver1] from group[group3]
   Kick host[sdbserver2] from group[group3]
   Kick host[sdbserver5] from group[group3]
   Update kicked group[group3] to sdbserver3:11800 succeed
   Kick host[sdbserver1] from group[SYSCoord]
   Kick host[sdbserver2] from group[SYSCoord]
   Kick host[sdbserver5] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver3:11800 succeed
   Update sdbserver3:11800 catalog's info succeed
   Update sdbserver3:11800 catalog's readonly prop succeed
   Stop 11800 succeed in sdbserver4
   Start 11800 by standalone succeed in sdbserver4
   Change sdbserver4:11800 to standalone succeed
   Kick host[sdbserver1] from group[SYSCatalogGroup]
   Kick host[sdbserver2] from group[SYSCatalogGroup]
   Kick host[sdbserver5] from group[SYSCatalogGroup]
   Update kicked group[SYSCatalogGroup] to sdbserver4:11800 succeed
   Kick host[sdbserver1] from group[group1]
   Kick host[sdbserver2] from group[group1]
   Kick host[sdbserver5] from group[group1]
   Update kicked group[group1] to sdbserver4:11800 succeed
   Kick host[sdbserver1] from group[group2]
   Kick host[sdbserver2] from group[group2]
   Kick host[sdbserver5] from group[group2]
   Update kicked group[group2] to sdbserver4:11800 succeed
   Kick host[sdbserver1] from group[group3]
   Kick host[sdbserver2] from group[group3]
   Kick host[sdbserver5] from group[group3]
   Update kicked group[group3] to sdbserver4:11800 succeed
   Kick host[sdbserver1] from group[SYSCoord]
   Kick host[sdbserver2] from group[SYSCoord]
   Kick host[sdbserver5] from group[SYSCoord]
   Update kicked group[SYSCoord] to sdbserver4:11800 succeed
   Update sdbserver4:11800 catalog's info succeed
   Update sdbserver4:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver3:11803,sdbserver4:11803 succeed
   Restart all nodes succeed in sdbserver3
   Restart all nodes succeed in sdbserver4
   Restart all host nodes succeed
   Done
   ```

   此时城市2(sdbserver3、sdbserver4)已完成切换，成为独立的业务集群，并且可以正常对外提供服务。

3. 城市1和城市3故障恢复后，在 sdbserver1 修改 `cluster_opr.js` 中的配置项

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver3", "sdbserver4" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver2", "sdbserver5" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 2 ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
   ```

   设置 ACTIVE=false，使分裂后的三副本集群进入“只读”模式，只有城市2的两副本集群具有“写”功能，从而避免了脑裂（brain-split）的情况。

4. 开启数据节点自动全量同步

   如果 SUB2 中节点是异常终止的，重新启动节点时必须通过全量同步来恢复数据。数据节点参数设置 dataerrorop=2，会阻止全量同步的发生，导致数据节点无法启动。因此，主中心（SUB1）执行分裂操作之前，需要在所有数据节点的配置文件 `sdb.conf` 中设置 dataerrorop=1，才能顺利启动数据节点。

5. 在 sdbserver1 执行分裂

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh split.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to split cluster...
   Stop 11800 succeed in sdbserver1
   Start 11800 by standalone succeed in sdbserver1
   ...
   sdbserver1:11803,sdbserver2:11803,sdbserver5:11803 succeed
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver2
   Restart all nodes succeed in sdbserver5
   Restart all host nodes succeed
   Done
   ```

### 灾难修复

当城市1和城市3的所有故障都完成修复，需要将两个子网已分离的独立集群进行合并，恢复最初的状态。

1. 城市2执行合并

   ```lang-bash
   [sdbadmin@sdbserver4 dr_ha]$ sh merge.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
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
   Update sdbserver3:11800 catalog's readonly prop succeed
   Stop 11800 succeed in sdbserver4
   Start 11800 by standalone succeed in sdbserver4
   Change sdbserver4:11800 to standalone succeed
   Restore group[SYSCatalogGroup] to sdbserver4:11800 succeed
   Restore group[group1] to sdbserver4:11800 succeed
   Restore group[group2] to sdbserver4:11800 succeed
   Restore group[group3] to sdbserver4:11800 succeed
   Restore group[SYSCoord] to sdbserver4:11800 succeed
   Restore sdbserver4:11800 catalog's info succeed
   Update sdbserver4:11800 catalog's readonly prop succeed
   Update all nodes's catalogaddr to sdbserver1:11803,sdbserver2:11803,sdbserver3:11803,sdbserver4:11803,sdbserver5:11803 succeed
   Restart all nodes succeed in sdbserver4
   Restart all nodes succeed in sdbserver3
   Restart all host nodes succeed
   Done
   ```

2. 城市1和城市3（SUB2） 执行合并

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh merge.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to merge cluster...
   ...
   Restart all nodes succeed in sdbserver1
   Restart all nodes succeed in sdbserver2
   Restart all nodes succeed in sdbserver5
   Restart all host nodes succeed
   Done
   ```
3. 关闭数据节点自动全量同步

   当合并操作完成并且 SUB2 和 SUB1 的数据追平，后续不再需要数据节点的自动全量同步，因此需要将所有数据节点的 dataerrorop 参数改回最初的设置，即 dataerrorop=2。

4. 再次执行初始化，恢复集群最初状态

   由于合并之后，集群中主节点全部分布在灾备集群（SUB2）中，因此需要再次执行初始化操作，将主节点重新分布到主中心（SUB1）中。

   此时的子网划分如下：

   | 子网 | 主机                                           |
   | :--- | :--------------------------------------------- |
   | SUB1 | sdbserver1                                     |
   | SUB2 | sdbserver2、sdbserver3、sdbserver4、sdbserver5 |

   在 sdbserver1 修改 `cluster_opr.js` 中的配置项

   ```lang-javascript
   if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1" ] ; }
   if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver2", "sdbserver3", "sdbserver4", "sdbserver5" ] ; }
   if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
   if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
   if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
   if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
   ```

   在 sdbserver1 执行初始化

   ```lang-bash
   [sdbadmin@sdbserver1 dr_ha]$ sh init.sh 
   Begin to check args...
   Done
   Begin to check enviroment...
   Done
   Begin to init cluster...
   Start to copy init file to cluster host
   Copy init file to sdbserver3 success
   Copy init file to sdbserver4 success
   Copy init file to sdbserver5 success
   Copy init file to sdbserver2 success
   Done
   Begin to update catalog and data nodes's config...Done
   Begin to reload catalog and data nodes's config...Done
   Begin to reelect all groups...Done
   Done
   ```
   > **Note:**
   >
   > 再次执行初始化操作之前，需要先删除 SequoiaDB 安装目录下的 `datacenter_init.info` 文件，否则执行 `init.sh` 会提示如下错误：
   >
   >Already init. If you want to re-init, you should to remove the file: /opt/sequoiadb/datacenter_init.info



[^_^]:
    本文使用到的所有链接

[twodatacenter_usage]:manual/Distributed_Engine/Maintainance/HA_DR/disaster_recovery_tool.md#同城双中心
[twodatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/twodatacenter.md
[threedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/threedatacenter.md
[twocity_threedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/twocity_threedatacenter.md
[threecity_fivedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/threecity_fivedatacenter.md
[split_merge]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md