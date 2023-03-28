[^_^]:
    同城双中心部署

本文档主要介绍在同城双中心的部署方案下，如何应对不同级别的灾难。

##灾难应对方案##

###节点故障###

当复制组中超过半数节点发生故障，该复制组将无法提供读写服务。针对该情况，用户需参考[节点故障场景][node_down]进行灾难恢复。如果故障节点未超过半数，用户仅需修复故障节点并恢复节点数据即可。

![单节点故障情况][single_breakdown]

###主中心故障###

当主中心发生故障，集群将失去半数以上的节点，导致无法对外提供读写服务。针对该情况，用户需参考[数据中心故障场景][center_down]进行灾难恢复。

![主中心故障情况][center_breakdown]

###灾备中心故障###

当灾备中心发生故障，主中心仍可提供读写服务。针对该情况，用户仅需修复故障中心并恢复节点数据即可。

![灾备中心故障情况][sub2_breakdown]

###数据中心网络故障###

当数据中心发生网络故障，集群仍可提供读写服务。针对该情况，用户仅需修复网络故障并恢复节点数据即可。

![同城网络故障情况][net_breakdown]

##灾难恢复##

SequoiaDB 巨杉数据库提供[容灾切换合并工具][split_merge]，用于对节点故障场景和数据中心故障场景进行灾难恢复。下述以 SequoiaDB 安装目录 `/opt/sequoiadb/`、集群鉴权用户名“sdbadmin”和用户密码“sdbadmin”为例，介绍不同场景的灾难恢复步骤。

###节点故障场景###

下述以复制组 group1 不可用为例，介绍节点故障场景的具体恢复步骤。

1. 选择一台未发生故障的主机 `sdbserver`，切换至容灾工具目录

    ```lang-bash
    $ cd /opt/sequoiadb/tools/dr_ha
    ```

2. 修改配置文件 `cluster_opr.js`

    ```lang-bash
    $ vim cluster_opr.js
    ```

    修改内容如下：

    ```lang-javascript
    if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
    if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
    if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
    if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver" ] ; }
    if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver:11810" ] }
    if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
    if ( typeof(MINREPLICANUM) == "undefined" ) { MINREPLICANUM = 1 ; }
    if ( typeof(NEEDREELECT) == "undefined" ) { NEEDREELECT = false }
    if ( typeof(NEEDBROADCASTINITINFO) == "undefined" ) { NEEDBROADCASTINITINFO = true }
    ```

    >**Note:**
    >
    > 配置文件的参数说明可参考[容灾切换合并工具][split_merge]。

3. 剔除故障节点，剔除后复制组将恢复读写服务

    ```lang-bash
    $ sh detachGroupNode.sh
    ```

4. 修复故障节点

5. 将修复后的节点重新加入复制组

    ```lang-bash
    $ sh attachGroupNode.sh
    ```

6. 通过 SDB Shell 检查节点间数据是否一致

    ```lang-javascript
    > db.snapshot(SDB_SNAP_HEALTH, {GroupName: "group1"}, {IsPrimary: null, CompleteLSN: null})
    ```

    对比主备节点间字段 CompleteLSN 的值，如果保持一致则说明节点数据一致

    ```lang-json
    {
      "IsPrimary": true,
      "CompleteLSN": 228
    }
    {
      "IsPrimary": false,
      "CompleteLSN": 228
    }
    {
      "IsPrimary": false,
      "CompleteLSN": 228
    }
    ```

    >**Note:**
    >
    > 如果主备节点间字段 CompleteLSN 的值不一致，用户需等待片刻，待节点数据同步后再次检查节点数据是否一致。

7. 在当前复制组中重新选主

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.reelect({Seconds: 60})
    ```

###数据中心故障场景###

当数据中心故障导致集群不可用，需要通过以下步骤进行恢复：

1. 划分子网
2. 集群分裂
3. 集群合并
4. 集群信息初始化

下述以主中心整体故障为例，介绍数据中心故障场景的具体恢复步骤。

**划分子网**

将正常机房划分为 SUB1，故障机房划分为 SUB2

| 子网 | 主机                   |
| ---- | ---------------------- |
| SUB1 | sdbserver3             |
| SUB2 | sdbserver1、sdbserver2 |

**集群分裂**

集群分裂操作用于将灾备中心从集群中分裂，使其成为具备读写功能的独立集群。分裂完成后，将由灾备中心对外提供服务。

1. 开启自动全量同步，避免节点重启失败

    逐一修改所有故障节点的配置文件 `sdb.conf`

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/<端口号>/sdb.conf
    ```

    修改内容如下：

    ```lang-ini
    ...
    dataerrorop=1
    ...
    ```

2. 选择 SUB1 的主机，切换至容灾工具目录


    ```lang-bash
    $ cd /opt/sequoiadb/tools/dr_ha
    ```

3. 修改配置文件 `cluster_opr.js`

    ```lang-bash
    $ vim cluster_opr.js
    ```

    修改内容如下：

    ```lang-javascript
    if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver3" ] ; }
    if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver2" ] ; }
    if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver3:11810" ] }
    if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
    if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
    ```

4. 执行分裂操作

    ```lang-bash
    $ sh split.sh
    ```

5. 修复故障中心

6. 选择 SUB2 的任意一台主机，切换至容灾工具目录

    ```lang-bash
    $ cd /opt/sequoiadb/tools/dr_ha
    ```

7. 修改配置文件 `cluster_opr.js`

    ```lang-bash
    $ vim cluster_opr.js
    ```

    修改内容如下：

    ```lang-javascript
    if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver3" ] ; }
    if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver1", "sdbserver2" ] ; }
    if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
    if ( typeof(CURSUB) == "undefined" ) { CURSUB = 2 ; }
    if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
    ```

8. 执行分裂操作

    ```lang-bash
    $ sh split.sh 
    ```

**集群合并**

1. 选择集群中的任意一台主机，执行合并操作

    ```lang-bash
    $ sh merge.sh 
    ```

2. 通过 SDB Shell 检查节点间数据是否一致

    ```lang-javascript
    > db.snapshot(SDB_SNAP_HEALTH, {}, {NodeName: null, IsPrimary: null, CompleteLSN: null})
    ```

    对比主备节点间字段 CompleteLSN 的值，如果保持一致则说明节点间数据一致

    ```lang-json
    ...
    {
      "NodeName": "sdbserver1:11820",
      "IsPrimary": true,
      "CompleteLSN": 80148
    }
    {
      "NodeName": "sdbserver2:11820",
      "IsPrimary": false,
      "CompleteLSN": 80148
    }
    {
      "NodeName": "sdbserver3:11820",
      "IsPrimary": false,
      "CompleteLSN": 80148
    }
    ...
    ```

    >**Note:**
    >
    > 如果主备节点间字段 CompleteLSN 的值不一致，用户需等待片刻，待节点数据同步后再次检查节点数据是否一致。

3. 关闭自动全量同步

    ```lang-javascript
    > db.updateConf({dataerrorop: 2})
    ```

**集群信息初始化**

集群信息初始化操作用于将主节点分配至主中心，以恢复集群初始状态。执行该操作前，用户需转移或删除集群部署信息文件 `datacenter_init.info`，该文件位于 SequoiaDB 安装目录。

1. 根据数据中心划分子网，主中心为 SUB1，灾备中心为 SUB2

    | 子网 | 主机                   |
    | ---- | ---------------------- |
    | SUB1 | sdbserver1、sdbserver2 |
    | SUB2 | sdbserver3 |

2. 选择 SUB1 的任意一台主机，切换至容灾工具目录

    ```lang-bash
    $ cd /opt/sequoiadb/tools/dr_ha
    ```

3. 修改配置文件 `cluster_opr.js`

    ```lang-bash
    $ vim cluster_opr.js
    ```

    修改内容如下：

    ```lang-javascript
    if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
    if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
    if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
    if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1", "sdbserver2" ] ; }
    if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver3" ] ; }
    if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
    if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
    if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
    if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
    if ( typeof(NEEDBROADCASTINITINFO) == "undefined" ) { NEEDBROADCASTINITINFO = false }
    ```

4. 集群信息初始化

    ```lang-bash
    $ sh init.sh 
    ```

5. 选择 SUB2 的主机，切换至容灾工具目录

    ```lang-bash
    $ cd /opt/sequoiadb/tools/dr_ha
    ```

6. 修改配置文件 `cluster_opr.js`

    ```lang-bash
    $ vim cluster_opr.js
    ```

    修改内容如下：

    ```lang-javascript
    if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
    if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "sdbadmin" ; }
    if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "sdbadmin" ; }
    if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "sdbserver1", "sdbserver2" ] ; }
    if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "sdbserver3" ] ; }
    if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "sdbserver1:11810" ] }
    if ( typeof(CURSUB) == "undefined" ) { CURSUB = 2 ; }
    if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
    if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = false ; }
    if ( typeof(NEEDBROADCASTINITINFO) == "undefined" ) { NEEDBROADCASTINITINFO = true }
    ```

7. 集群信息初始化

    ```lang-bash
    $ sh init.sh 
    ```

[^_^]:
    本文使用到的所有链接
[twodatacenter_cluster]:images/Distributed_Engine/Maintainance/HA_DR/twodatacenter_cluster.png
[single_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/single_breakdown.png
[sub2_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/sub2_breakdown.png
[net_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/net_breakdown.png
[center_breakdown]:images/Distributed_Engine/Maintainance/HA_DR/center_breakdown.png
[recovery]:Distributed_Engine/Maintainance/HA_DR/twodatacenter.md#灾难恢复
[split_merge]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md
[replication]:manual/Distributed_Engine/Architecture/Replication/architecture.md
[node_down]:manual/Distributed_Engine/Maintainance/HA_DR/twodatacenter.md#节点故障场景
[center_down]:manual/Distributed_Engine/Maintainance/HA_DR/twodatacenter.md#数据中心故障场景