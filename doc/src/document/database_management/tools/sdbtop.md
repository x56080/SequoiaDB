sdbtop 是 SequoiaDB 性能监控工具。通过 sdbtop，可以监控和查看集群中各个节点的监视信息。

##选项##

| 参数          | 缩写 | 描述                   |
| ------------- | ---- | ---------------------- |
| --help        | -h   | 返回基本帮助和用法文本 |
| --confpath    | -c   | 指定 sdbtop 的配置文件路径，sdbtop 界面形态以及输出字段都依赖该文件（缺省使用默认配置文件） |
| --hostname    | -i   | 指定需要监控的主机名   |
| --servicename | -s   | 指定监控的端口服务名   |
| --usrname     | -u   | 指定数据库用户名       |
| --password    | -p   | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码 |
| --cipher      |      | 是否使用密文模式输入密码，默认为 false，不使用密文模式输入密码。关于密文模式的介绍，详细可参考[密码管理](database_management/security/system_security.md) |
| --token       |      | 指加密令牌           |
| --cipherfile  |      | 指定密文文件路径，默认为 `~/sequoiadb/passwd` |
| --ssl         |      | 是否使用 SSL 连接，默认为 false，不使用 SSL 连接 |

>   **Note:**
>
>   *   对于 Ubuntu 等系统，需要安装 Ncurses 库，否则将会提示“Error opening terminal: TERM”
>       *    方式一： 联网安装
>
>            ```lang-bash
>            $ sudo apt-get install libncurses5-dev
>            ```
>
>       *    方式二： 源码安装
>
>            解压源码包：
>
>            ```lang-bash
>            $ tar -xvzf ncurses-5.5.tar.gz
>            ```
>
>            进入 ncurses-5.5 目录
>
>            ```lang-bash
>            $ ./configure
>            $ sudo make && make install
>            ```
>   *   若 Ncurses 库安装完成后仍提示“Error opening terminal: TERM”，则尝试以下解决方案：
>       *    创建软连接
>
>            ```lang-bash
>            $ sudo mkdir -p /usr/share/terminfo/x
>            $ cd /usr/share/terminfo/x
>            $ sudo ln -s /lib/terminfo/x/xterm xterm
>            ```

##使用方法##

在下面的例子，sdbtop 使用配置文件为`/opt/sequoiadb/conf/samples/sdbtop.xml`，监控主机名为 hostname1，端口服务名为 11810，用户名为 test，密码为 test 的数据库集群中的一个节点。

```lang-bash
$ sdbtop -c /opt/sequoiadb/conf/samples/sdbtop.xml -i hostname1 -s 11810 -u test -p test
```

接着进入主窗口：

```lang-bash
refresh= 3 secs           sdbtop 1.0       snapshotMode: GLOBAL
displayMode: ABSOLUTE     Main Window      snapshotModeInput: NULL
hostname: hostname1                        filtering Number: 0
servicename: 11810                         sortingWay: NULL sortingField: NULL
usrName: test                              Refresh: F5, Quit: q, Help: h

 #####  ######  ######  #######  #####  ######   For help type h or ...
#       #     # #     #    #    #     # #     #  sdbtop -h: usage
#       #     # #     #    #    #     # #     #
 #####  #     # ######     #    #     # ######
      # #     # #     #    #    #     # #
      # #     # #     #    #    #     # #
 #####  ######  ######     #     #####  #

SDB Interactive Snapshot Monitor V2.0
Use these keys to navigate:
  m   -  Main Window            s   -  Sessions               c   -  CollectionSpaces
  t   -  System                 d   -  Database               G   -  GLOBAL_SNAPSHOT
  g   -  GROUP_SNAPSHOT         n   -  NODE_SNAPSHOT          r   -  reset refreshInterval
  A   -  Ascending order        D   -  Descending order       C   -  filter condition
  Q   -  no filter condition    N   -  filter number          W   -  no filter number
Licensed Materials - Property of SequoiaDB
Copyright SequoiaDB Corp. 2013-2014 All Rights Reserved.
```

>   **Note:**
>
>   在主窗口中按“**h**”键可以查看所有工具支持的按键

**主窗口按键说明**

| 按键    | 描述                                                 |
| ------- | ---------------------------------------------------- |
| m       | 返回主窗口                                           |
| s       | 列出数据库节点上的所有会话                           |
| c       | 列出数据库节点上的所有集合空间                       |
| t       | 列出数据库节点上的系统资源使用情况                   |
| d       | 列出数据库节点的数据库监视信息                       |
| G       | global_snapshot，监控所有的数据节点组                |
| g       | group_snapshot，指定监控某个数据节点组               |
| n       | node_snapshot，列出指定的数据库节点的监视信息        |
| r       | 设置刷屏的时间间隔，单位：秒                         |
| A       | 将监视信息按照某列进行顺序排序                       |
| D       | 将监视信息按照某列进行逆序排序                       |
| C       | 将监视信息按照某个条件进行筛选                       |
| Q       | 返回没有使用条件进行筛选前的监视信息                 |
| N       | 将监视信息中对应行号的记录过滤不显示                 |
| W       | 返回没有使用行号进行过滤前的监视信息                 |
| h       | 查看使用帮助                                         |
| <Esc>   | 取消已进入的操作                                     |
| <Enter> | 返回上一次监视界面，（在已进入 help 帮助输出中有效） |
| <F5>    | 强制刷新后台监视信息                                 |
| <       | 向左移动，以查看隐藏的左边列的监视信息               |
| >       | 向右移动，以查看隐藏的右边列的监视信息               |
| q       | 退出程序                                             |
| <Tab>   | 切换数据计算的模式（绝对值，平均值，差值三个模式）   |

**例子**

1.  进入主窗口后，按“s”键，列出数据库节点的所有会话信息

    ```lang-bash
    refresh= 3 secs              sdbtop 1.0         snapshotMode: GLOBAL
    displayMode: ABSOLUTE        Sessions           snapshotModeInput: NULL
    hostname: hostname1                             filtering Number: 0
    servicename: 11810                              sortingWay: NULL sortingField: NULL
    usrName: test                                   Refresh: F5, Quit: q, Help: h
    
    SessionID                           TID Type               Name
    ------------------------------   ----------    ------------------    ------------------------------
    1  hostname1:11820:1                10732 LogWriter          ""
    2  hostname1:11820:10               10741 Task               Job[Prefetcher]
    3  hostname1:11820:11               10742 Task               Job[Prefetcher]
    4  hostname1:11820:12               10743 Task               Job[Prefetcher]
    5  hostname1:11820:13               10744 Cluster            ""
    6  hostname1:11820:14               10745 ClusterShard       ""
    7  hostname1:11820:15               10746 ClusterLogNotify   ""
    8  hostname1:11820:16               10747 ShardReader        ""
    9  hostname1:11820:17               10748 ReplReader         ""
    10  hostname1:11820:18              10749 SyncClockWorker    ""
    11  hostname1:11820:19              10750 TCPListener        ""
    12  hostname1:11820:2               10733 DpsRollback        ""
    13  hostname1:11820:20              10751 RestListener       ""
    14  hostname1:11820:21              10752 Task               Job[PageCleaner]
    15  hostname1:11820:3               10734 Task               Job[Prefetcher]
    16  hostname1:11820:4               10735 Task               Job[Prefetcher]
    17  hostname1:11820:42              10847 ReplAgent          NodeID:1000,TID:1,Start:active
    18  hostname1:11820:5               10736 Task               Job[Prefetcher]
    19  hostname1:11820:59              23263 ShardAgent         NetID:1,TID:23262
    20  hostname1:11820:6               10737 Task               Job[Prefetcher]
    21  hostname1:11820:7               10738 Task               Job[Prefetcher]
    22  hostname1:11820:8               10739 Task               Job[Prefetcher]
    ```

2.  按“Tab”键，可以看到屏幕左上方的“displayMode”的值会发生切换

3.  按“r”键，在屏幕最下方输入“2”，回车，设置刷新间隔时间，可以看到屏幕左上方的“refresh”的值变为 2

    ```lang-bash
    refresh= 2 secs              sdbtop 1.0         snapshotMode: GLOBAL
    displayMode: ABSOLUTE        Sessions           snapshotModeInput: NULL
    hostname: hostname1                             filtering Number: 0
    servicename: 11810                              sortingWay: NULL sortingField: NULL
    usrName: test                                   Refresh: F5, Quit: q, Help: h
    
    SessionID                           TID Type               Name
    ------------------------------   ----------    ------------------    ------------------------------
    1  hostname1:11820:1                10732 LogWriter          ""
    2  hostname1:11820:10               10741 Task               Job[Prefetcher]
    3  hostname1:11820:11               10742 Task               Job[Prefetcher]
    4  hostname1:11820:12               10743 Task               Job[Prefetcher]
    5  hostname1:11820:13               10744 Cluster            ""
    6  hostname1:11820:14               10745 ClusterShard       ""
    7  hostname1:11820:15               10746 ClusterLogNotify   ""
    8  hostname1:11820:16               10747 ShardReader        ""
    9  hostname1:11820:17               10748 ReplReader         ""
    10  hostname1:11820:18              10749 SyncClockWorker    ""
    11  hostname1:11820:19              10750 TCPListener        ""
    12  hostname1:11820:2               10733 DpsRollback        ""
    13  hostname1:11820:20              10751 RestListener       ""
    14  hostname1:11820:21              10752 Task               Job[PageCleaner]
    15  hostname1:11820:3               10734 Task               Job[Prefetcher]
    16  hostname1:11820:4               10735 Task               Job[Prefetcher]
    17  hostname1:11820:42              10847 ReplAgent          NodeID:1000,TID:1,Start:active
    18  hostname1:11820:5               10736 Task               Job[Prefetcher]
    19  hostname1:11820:59              23263 ShardAgent         NetID:1,TID:23262
    20  hostname1:11820:6               10737 Task               Job[Prefetcher]
    21  hostname1:11820:7               10738 Task               Job[Prefetcher]
    22  hostname1:11820:8               10739 Task               Job[Prefetcher]
    ```

4.  按“A”键，并输入“TID”，列表结果按照 TID 进行顺序排序

    ```lang-bash
    refresh= 2 secs              sdbtop 1.0         snapshotMode: GLOBAL
    displayMode: ABSOLUTE        Sessions           snapshotModeInput: NULL
    hostname: hostname1                             filtering Number: 0
    servicename: 11810                              sortingWay: NULL sortingField: NULL
    usrName: test                                   Refresh: F5, Quit: q, Help: h
    
    SessionID                           TID Type               Name
    ------------------------------   ----------    ------------------    ------------------------------
    1  hostname1:11820:1                10732 LogWriter          ""
    2  hostname1:11820:10               10741 Task               Job[Prefetcher]
    3  hostname1:11820:11               10742 Task               Job[Prefetcher]
    4  hostname1:11820:12               10743 Task               Job[Prefetcher]
    5  hostname1:11820:13               10744 Cluster            ""
    6  hostname1:11820:14               10745 ClusterShard       ""
    7  hostname1:11820:15               10746 ClusterLogNotify   ""
    8  hostname1:11820:16               10747 ShardReader        ""
    9  hostname1:11820:17               10748 ReplReader         ""
    10  hostname1:11820:18              10749 SyncClockWorker    ""
    11  hostname1:11820:19              10750 TCPListener        ""
    12  hostname1:11820:2               10733 DpsRollback        ""
    13  hostname1:11820:20              10751 RestListener       ""
    14  hostname1:11820:21              10752 Task               Job[PageCleaner]
    15  hostname1:11820:3               10734 Task               Job[Prefetcher]
    16  hostname1:11820:4               10735 Task               Job[Prefetcher]
    17  hostname1:11820:42              10847 ReplAgent          NodeID:1000,TID:1,Start:active
    18  hostname1:11820:5               10736 Task               Job[Prefetcher]
    19  hostname1:11820:59              23263 ShardAgent         NetID:1,TID:23262
    20  hostname1:11820:6               10737 Task               Job[Prefetcher]
    21  hostname1:11820:7               10738 Task               Job[Prefetcher]
    please input the displayName which need order by asc : TID
    ```

5.  按“N”键，并输入“1”，列表中将原来行号为 1 的记录过滤不显示

6.  按“W”键，返回没有按行号进行过滤前的列表信息

7.  按“C”键，并输入“TID:10732”进行筛选，则只显示 TID 值为 10732 的记录

    ```lang-bash
    refresh= 2 secs              sdbtop 1.0         snapshotMode: GLOBAL
    displayMode: ABSOLUTE        Sessions           snapshotModeInput: NULL
    hostname: hostname1                             filtering Number: 0
    servicename: 11810                              sortingWay: NULL sortingField: NULL
    usrName: test                                   Refresh: F5, Quit: q, Help: h
    
    SessionID                           TID Type               Name
    ------------------------------   ----------    ------------------    ------------------------------
    1  hostname1:11820:1                10732 LogWriter          ""
    2  hostname1:11820:10               10741 Task               Job[Prefetcher]
    3  hostname1:11820:11               10742 Task               Job[Prefetcher]
    4  hostname1:11820:12               10743 Task               Job[Prefetcher]
    5  hostname1:11820:13               10744 Cluster            ""
    6  hostname1:11820:14               10745 ClusterShard       ""
    7  hostname1:11820:15               10746 ClusterLogNotify   ""
    8  hostname1:11820:16               10747 ShardReader        ""
    9  hostname1:11820:17               10748 ReplReader         ""
    10  hostname1:11820:18              10749 SyncClockWorker    ""
    11  hostname1:11820:19              10750 TCPListener        ""
    12  hostname1:11820:2               10733 DpsRollback        ""
    13  hostname1:11820:20              10751 RestListener       ""
    14  hostname1:11820:21              10752 Task               Job[PageCleaner]
    15  hostname1:11820:3               10734 Task               Job[Prefetcher]
    16  hostname1:11820:4               10735 Task               Job[Prefetcher]
    17  hostname1:11820:42              10847 ReplAgent          NodeID:1000,TID:1,Start:active
    18  hostname1:11820:5               10736 Task               Job[Prefetcher]
    19  hostname1:11820:59              23263 ShardAgent         NetID:1,TID:23262
    20  hostname1:11820:6               10737 Task               Job[Prefetcher]
    21  hostname1:11820:7               10738 Task               Job[Prefetcher]
    please input the filter condition : TID:10732
    ```

8.  按“Q”键，返回没有按照筛选条件前的列表信息

9.  按“<”或者“>”键，可以查看隐藏在左边或者右边的列
