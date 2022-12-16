本文档将介绍 trace 跟踪相关的命令、开启 trace 日志收集、断点跟踪功能、输出 trace 日志和 trace 日志格式化。

##trace 相关的命令##

| 操作       | 说明           | 
| ---------- | -------------- |
| [traceOn][traceOn] | 开启数据库引擎跟踪功能<br>可同时指定多个过滤选项，当指定断点跟踪时，会在流程进入断点时挂起相应的线程 |
| [traceResume][traceResume] | 重新开启断点跟踪程序<br>能恢复所有被断点跟踪挂起的线程，并删除所有断点 |
| [traceStatus][traceStatus] | 查看当前程序跟踪的状态 |
| [traceOff][traceOff] | 关闭数据库引擎跟踪功能，并将跟踪情况导出为二进制文件 |
| [traceFmt][traceFmt] | 解析 trace 日志，并按照指定的格式化类型输出文件 |

##开启 trace 日志收集##

用户通过 traceOn 命令可以开启 trace 功能，通过 traceStatus 可以了解 trace 的状态，通过 traceResume 可以恢复被挂起的线程并继续跟踪。

开启 trace 日志收集需要先连接到相应节点上。

* 跟踪所有模块

    ```lang-javascript
    > db.traceOn(1000)
    ```

* 按模块过滤

    ```lang-javascript
    > var option = new SdbTraceOption().components("dms")
    > db.traceOn(1000, option)
    ```

* 按线程过滤

    ```lang-javascript
    > var option = new SdbTraceOption().tids(4650)
    > db.traceOn(1000, option)
    ```

* 按函数名过滤

    ```lang-javascript
    > var option = new SdbTraceOption().functionNames("_dmsTempSUMgr::init")
    > db.traceOn(1000, option)
    ```

* 按线程类型过滤

    ```lang-javascript
    > var option = new SdbTraceOption().threadTypes("ShardAgent")
    > db.traceOn(1000, option)
    ```

* 断点跟踪

    ```lang-javascript 
    > var option = new SdbTraceOption().breakPoints("_dmsTempSUMgr::init")
    > db.traceOn(1000, option)
    ```

* 恢复断点执行

    ```lang-javascript
    > db.traceResume()
    ```

* 模块和线程组合过滤

    ```lang-javascript
    > var option = new SdbTraceOption().components(["cls", "dms", "mth"]).tids([12712, 12713, 12714])
    > db.traceOn(1000, option)
    ```

* 查看跟踪状态

    ```lang-javascript
    > db.traceStatus()
    ```

##trace 日志输出##

关闭跟踪，并将缓冲区中的 trace 日志以二进制格式输出到指定的文件中

```lang-javascript
> db.traceOff('tracedump.dat')
```

文件路径为绝对路径时，需要有相关目录的权限；为相对路径时，则输出到节点日志目录下面。

##trace 日志格式化##

trace 日志格式化用于将 trace 的输出日志格式化为可读的形式。

- 输出分析文件

    ```lang-bash
    > traceFmt(0, ${dbpath} + '/diaglog/tracedump.dat', ${dbpath} + '/diaglog/trace_fmt')
    ```

    按线程进行组织输出流程文件，并且针对结果作初步分析

    ```lang-text
    -rw-r----- 1 root     root                  0 Mar 25 20:35 trace_flw.error
    -rw-r----- 1 root     root                880 Mar 25 20:35 trace_flw.except
    -rw-r----- 1 root     root             129033 Mar 25 20:35 trace_flw.flw
    -rw-r----- 1 root     root               6497 Mar 25 20:35 trace_flw.funcRecord.csv
    -rw-r----- 1 root     root                 40 Mar 25 20:35 trace_flw.version
    ```
   
    其中 `trace_flw.flw` 输出为:
   
    ```lang-text
    1231:  _pmdLocalSession::_processMsg Entry(369): 2019-03-25-20.32.32.543809
    1232:  | _pmdLocalSession::_processMsg(370): 2019-03-25-20.32.32.543810
    1233:  | _pmdCoordProcessor::processMsg Entry(1756): 2019-03-25-20.32.32.543812
    1234:  | | _pmdCoordProcessor::_processCoordMsg Entry(1352): 2019-03-25-20.32.32.543813
    1235:  | | | _pmdCoordProcessor::_processCoordMsg(1354): 2019-03-25-20.32.32.543814
    1236:  | | | _netFrame::syncSend Entry(790): 2019-03-25-20.32.32.543844
    1237:  | | | | _netEventHandler::syncSend Entry(465): 2019-03-25-20.32.32.543847
    1238:  | | | | _netEventHandler::syncSend Exit(508): 2019-03-25-20.32.32.543862
    1239:  | | | _netFrame::syncSend Exit(821): 2019-03-25-20.32.32.543863
    1240:  | | | _pmdEDUCB::isInterrupted Entry(730): 2019-03-25-20.32.32.543867
    1241:  | | | | _pmdEDUCB::isInterrupted(778): 2019-03-25-20.32.32.543868
    1242:  | | | _pmdEDUCB::isInterrupted Exit(779): 2019-03-25-20.32.32.543869
    1263:  | | | _pmdEDUCB::delTransaction Entry(1066): 2019-03-25-20.32.32.545389  (1520)
    1264:  | | | _pmdEDUCB::delTransaction Exit(1072): 2019-03-25-20.32.32.545391
    1265:  | | | _pmdCoordProcessor::_processCoordMsg(1582): 2019-03-25-20.32.32.545398
    1266:  | | _pmdCoordProcessor::_processCoordMsg Exit(1617): 2019-03-25-20.32.32.545399
    1267:  | _pmdCoordProcessor::processMsg Exit(1778): 2019-03-25-20.32.32.545400
    1268:  | ossSocket::send Entry(368): 2019-03-25-20.32.32.545402
    1269:  | ossSocket::send Exit(492): 2019-03-25-20.32.32.545426
    1270:  _pmdLocalSession::_processMsg Exit(430): 2019-03-25-20.32.32.545428
    ```
   
    `trace_flw.funcRecord.csv` 输出为:
   
    ```lang-text
    name, count, avgcost, min, maxIn2OutCost, maxCurrentCost, first, second, third, fourth, fifth
    _clsGroupItem::getNodeID,1,1.0,1,1,1,"(425, 1, 1, 1)"
    _clsGroupItem::getNodeInfo,1,3.0,3,4,3,"(427, 4, 3, 1)"
    msgCheckBuffer,2,5.0,5,5,5,"(98, 5, 5, 4)","(250, 5, 5, 4)"
    msgExtractInsert,2,2.0,2,2,2,"(802, 2, 2, 1)","(1066, 2, 2, 1)"
    msgBuildQueryMsg,2,4.5,4,10,5,"(95, 10, 5, 2)","(247, 9, 4, 1)"
    msgExtractQuery,11,2.1,2,3,3,"(83, 3, 3, 2)","(87, 2, 2, 1)","(91, 2, 2, 1)","(235, 2, 2, 1)","(239, 2, 2, 1)"
    msgExtractGetMore,2,0.0,0,0,0,"(533, 0, 0, 0)","(634, 0, 0, 0)"
    msgFillGetMoreMsg,4,0.2,0,1,1,"(570, 1, 1, 1)","(176, 0, 0, 0)","(328, 0, 0, 0)","(482, 0, 0, 0)"
    _netFrame::_getHandle,1,2.0,2,2,2,"(442, 2, 2, 2)"
    _netFrame::syncSend,1,3.0,3,20,3,"(441, 20, 3, 1)"
    _netFrame::syncSend,8,3.8,3,20,5,"(125, 20, 5, 4)","(572, 15, 4, 3)","(825, 20, 4, 3)","(1089, 20, 4, 3)","(1236, 19, 4, 3)"
    _netFrame::handleMsg,17,6.1,1,20,11,"(560, 11, 11, 11)","(848, 11, 11, 11)","(148, 11, 11, 10)","(300, 11, 11, 10)","(1112, 11, 11, 10)"
    ```
   
    `trace_flw.except` 输出为:
   
    ```lang-text
    _ossEvent::wait
    sequence: 756 tid: 8193  cost: 5000110 (5000110)
   
      756:  _ossEvent::wait Entry(64): 2019-03-25-20.32.02.602858
      872:  _ossEvent::wait Exit(93): 2019-03-25-20.32.07.602968      (5000110)
    ```
   
    `trace_flw.version` 输出为：
   
    ```lang-text
    SequoiaDB version : 3.2
    Release : 39719
    ```

- 输出 dump 记录信息

    ```lang-bash
    > traceFmt(1, ${dbpath} + '/diaglog/tracedump.dat', ${dbpath} + '/diaglog/trace_fmt' )
    ```

    按照日志收集顺序，顺序格式化成可读的文本形式

    ```lang-text
    -rw-r----- 1 sdbadmin sdbadmin_group   170129 Mar 25 20:32 tracedump.out
    -rw-r----- 1 root     root             215544 Mar 25 20:34 trace_fmt.fmt
    -rw-r----- 1 root     root                 40 Mar 25 20:34 trace_fmt.version
    ```

    `trace_fmt.fmt` 输出为:
  
    ```lang-text
    1: _rtnTraceStart::doit Exit(2736): 2019-03-25-19.40.34.097959
    tid: 8369, numArgs: 1
           arg0:   0
   
    2: _pmdCoordProcessor::_processCoordMsg Normal(1582): 2019-03-25-19.40.34.097966
    tid: 8369, numArgs: 1
           arg0:   0
   
    3: _pmdCoordProcessor::_processCoordMsg Exit(1617): 2019-03-25-19.40.34.097967
    tid: 8369, numArgs: 1
           arg0:   0
   
    4: _pmdCoordProcessor::processMsg Exit(1778): 2019-03-25-19.40.34.097968
    tid: 8369, numArgs: 1
           arg0:   0
   
    5: ossSocket::send Entry(368): 2019-03-25-19.40.34.097969
    tid: 8369, numArgs: 0
   
    6: ossSocket::send Exit(492): 2019-03-25-19.40.34.097992
    tid: 8369, numArgs: 1
           arg0:   0
   
    7: _pmdLocalSession::_processMsg Exit(430): 2019-03-25-19.40.34.097994
    tid: 8369, numArgs: 1
           arg0:   0
    ```

    版本文件输出同分析文件输出。

[^_^]: 
    本文使用的所有引用和链接
[traceOn]:manual/Manual/Sequoiadb_Command/Sdb/traceOn.md
[SdbTraceOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[traceStatus]:manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md
[traceOff]:manual/Manual/Sequoiadb_Command/Sdb/traceOff.md
[traceFmt]:manual/Manual/Sequoiadb_Command/Global/traceFmt.md
[traceResume]:manual/Manual/Sequoiadb_Command/Sdb/traceResume.md