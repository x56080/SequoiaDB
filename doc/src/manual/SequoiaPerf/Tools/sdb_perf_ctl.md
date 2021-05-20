[^_^]:
    目录名：实例管理工具

sdb_perf_ctl 是 SequoiaPerf 的实例管理工具。用户通过 sdb_perf_ctl 可以创建和管理实例。

##参数说明##

| 参数名 | 描述 | 是否必填 |
| ----  | ---- | -------- |
| -c    | 指定 SequoiaDB 集群的协调节点地址 | 是 |
| -e    | 指定 SequoiaPerf 页面服务器 ip | 是 |
| -u    | 指定 SequoiaDB 集群的鉴权用户名 | 否 |
| -w    | 指定 SequoiaDB 集群的鉴权用户密码 | 否 |
| -p    | 指定需要监听的端口，默认以 14000 端口起始，监听[14000,14020]端口；如果自定义监听端口，则需要保证延后的 20 个端口都可供 SequoiaPerf 实例使用 | 否 |
| -b    | 指定显示在 SequoiaPerf 页面上所连接 SequoiaDB 集群的业务名，方便用户区分 | 否 |
| -d    | 指定 Sequoia{erf 实例的数据目录，默认为 `SequoiaPerf 安装目录/instances` | 否 |
| --print | 打印日志信息  | 否 |

##使用说明##

运行 sdb_perf_ctl 的用户必须与安装 SequoiaPerf 时指定的用户一致。

- 创建实例

   bin/sdb_perf_ctl addinst <INSTNAME> <-c COORD_ADDR> <-e EXTERNAL_IP> [-p PORT] ] [-b BUSINESSNAME] [-d DATA_DIR] [-u SDB_USER] [-w SDB_MD5_PASSWORD] [--print]

   ```lang-bash
   $ bin/sdb_perf_ctl addinst SequoiaPerf1 -c 127.0.0.1:11810 -e 192.168.31.20 -b SequoiaPerf1 -d /tmp -u sdbtest -w 9c51816c474bb23a990ab22eddcd57a1 --print
   ```

- 启动实例

   bin/sdb_perf_ctl start <INSTNAME>

   ```lang-bash
   $ bin/sdb_perf_ctl start SequoiaPerf1
   ```

- 重启实例
  
   bin/sdb_perf_ctl restart <INSTNAME>

   ```lang-bash
   $ bin/sdb_perf_ctl restart SequoiaPerf1
   ```

- 停止实例

   bin/sdb_perf_ctl stop <INSTNAME>

   ```lang-bash
   $ bin/sdb_perf_ctl stop SequoiaPerf1
   ```

- 删除实例
  
   bin/sdb_perf_ctl delinst <INSTNAME>

   ```lang-bash
   $ bin/sdb_perf_ctl delinst SequoiaPerf1
   ```

   >**Note:**
   >
   > 删除 SequoiaPerf 实例后，该实例的所有日志、数据和配置将永久删除。

- 启动实例的[服务][Readme] 

   bin/sdb_perf_ctl start <INSTNAME> [SERVICE]

   ```lang-bash
   $ bin/sdb_perf_ctl start SequoiaPerf1 sequoiaperf_gateway
   ```

- 重启实例的服务

   bin/sdb_perf_ctl restart <INSTNAME> [SERVICE]

   ```lang-bash
   bin/sdb_perf_ctl restart SequoiaPerf1 sequoiaperf_gateway
   ```

- 停止实例的服务

   bin/sdb_perf_ctl stop <INSTNAME> [SERVICE]

   ```lang-bash
   $ bin/sdb_perf_ctl stop SequoiaPerf1 sequoiaperf_gateway
   ```

- 检查实例状态

   bin/sdb_perf_ctl status <INSTNAME>

   ```lang-bash
   $ bin/sdb_perf_ctl status
   ```

   >**Note:**
   >
   > 如果指定了实例名称，则表示查看指定实例中所有服务的状态。

- 重载服务

   bin/sdb_perf_ctl reload <INSTNAME> <SERVICE>

   ```lang-bash
   $ bin/sdb_perf_ctl reload SequoiaPerf1 sequoiaperf_exporter
   ```

   >**Note:**
   >
   > 用户修改相应的服务配置后，需要进行服务重载，更新 SequoiaPerf 相应服务的配置。




[^_^]:
    本文使用的所有引用及链接
[Readme]:manual/SequoiaPerf/Readme.md
