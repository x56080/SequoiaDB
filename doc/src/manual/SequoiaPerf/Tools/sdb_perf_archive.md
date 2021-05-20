[^_^]:
    目录名：归档管理工具

sdb_perf_archive 是 SequoiaPerf 的归档管理工具，用户通过该工具可以创建、删除和管理实例的归档。归档功能可以将历史数据保留，便于分析和诊断性能问题。

用户可以通过创建新实例，以新实例为恢复目标对归档进行恢复，但恢复后的数据为只读模式，不可操作。

##参数说明##

| 参数名 | 描述 | 是否必填 |
| ----  | ---- | -------- |
| -d    | 指定 SequoiaPerf 归档存放的目录 | 否 |
| -f    | 指定 SequoiaPerf 归档文件名 | 是 |
| -e    | 指定 Sequoiaperf 页面服务器 ip  | 是 |
| --print | 打印日志信息  | 否 |

##使用说明##

运行 sdb_perf_archive 的用户必须与安装 SequoiaPerf 时指定的用户一致。


- 创建归档

   sdb_perf_archive create <INSTNAME> [--print] [-d DIRECTORY]

   ```lang-bash
   $ sdb_perf_archive create perf1 -d /opt/sequoiaperf/mybackup/ --print
   ```

- 查看归档

   bin/sdb_perf_archive list <INSTNAME>  [--print]

   ```lang-bash
   $ sdb_perf_archive list perf1
   ```

- 删除归档
  
   bin/sdb_perf_archive delete -f FILE  [--print]

   ```lang-bash
   $ sdb_perf_archive delete -f 
   ```

- 恢复归档

   bin/sdb_perf_archive restore <INSTNAME> -f FILE  -e EXTERNAL_IP [-d DIRECTORY] [--print]

   ```lang-bash
   $ sdb_perf_archive restore perf3 -f 
   ```

   >**Note:**
   >
   > 用户可以在 SequoiaPerf 页面，以指定时间区间的方式查看指定的历史数据。


