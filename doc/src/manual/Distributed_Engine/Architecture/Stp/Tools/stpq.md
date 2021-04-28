sptq 是一个用于查询 STP 的时间，状态，配置等信息的工具。

> **Note:**
>
> stpq 默认查询本地的 STP 节点，用户也可以通过指定 hostname 参数查询其他机器上的 STP 节点。

## 权限需求

无

## 连接需求

需要连接到 STP 节点

## 选项

| 参数名 | 缩写 | 描述 |
| ---- | ---- | ---- |
| --help | -h | 返回 stpq 的用法和帮助 |
| --version | | 返回 stpq 的版本信息 |
| --hostname | -s | 指定需要连接的 STP 节点所在机器的主机名，默认值为"localhost" |
| --port | -p | 指定需要连接的 STP 节点的端口，默认值为 9622 |
| --time | | 查询 STP 节点当前的逻辑时间，单位为纳秒 |
| --timeus | | 查询 STP 节点当前的逻辑时间，单位为微秒 |
| --conf | | 查询 STP 节点的配置 |
| --meta | | 查询 STP 节点的元数据信息 |
| --servers | | 查询 STP 节点进行同步的 server 组的信息 |
| --syncclients | | 查询 STP 节点所在 STP 集群的时间同步信息 |
| --syncstatus | | 查询 STP 节点和当前同步源的同步信息 |
| --synchistory | | 查询 STP 节点和各个同步源的历史时间同步信息 |
| --count | -n | 指定连续打印的次数，默认值为 1 |
| --delay | -d | 指定连续打印时的间隔时间，单位为秒，默认值为 10 |

> **Note:**
>
> * 如果没有指定查询选项，将默认使用 --time 查询 STP 节点当前的逻辑时间
> * 如果指定了 --delay 但没有指定 --count，将不停地每隔一段指定的时间打印查询结果

## 查询输出

### 查询时间（纳秒）

--time 查询 STP 节点当前的逻辑时间，单位为纳秒

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| TimeStamp | 逻辑时间的时间部分，其中包含 second （秒）部分和 nanosec （纳秒）部分 |
| TimeError | 逻辑时间的时间容错误差部分，单位为纳秒 |

**示例**

```lang-bash
$ bin/stpq --time
Time:
   TimeStamp : ( 1590384470 second, 302563244 nanosec )
   TimeError : 1000000
```

### 查询时间（微秒）

--timeus 查询 STP 节点当前的逻辑时间，单位为微秒

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| TimeStamp | 逻辑时间的时间部分，单位为微秒 |
| TimeError | 逻辑时间的时间容错误差部分，单位为纳秒 |

**示例**

```lang-bash
$ bin/stpq --timeus
TimeUS:
   TimeStamp : 1590384549092550 microsec
   TimeError : 1000000
```

### 查询配置

--conf 查询 STP 节点的配置

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| port | STP 监听端口 |
| serverlist | STP 配置 server 列表 |
| role | STP 节点的角色 |
| syncinterval | STP 节点进行时间同步的间隔，单位为秒 |
| maxtimeerror | STP 节点可以容忍的最大时间误差，单位为微秒 |
| diaglevel | STP 节点打印诊断日志的级别 |

**示例**

```lang-bash
$ bin/stpq --conf
Config:
   port           : 9622
   serverlist     : sdbserver1:9622
   role           : server
   syncinterval   : 60
   maxtimeerror   : 50000
   diaglevel      : 3
```

### 查询元数据

--meta 查询 STP 节点的元数据信息

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| MetaSHMKey | STP 节点共享内存的键值 |
| Version | STP 节点元数据的版本号 |
| SyncInterval | STP 节点配置的同步间隔（通过 --syncinterval 配置） |
| SyncHWTime | STP 节点上次同步时的系统硬件时间，其中包含 second （秒）部分和 nanosec （纳秒）部分 |
| BaseHWTime | STP 节点启动时的系统硬件时间，其中包含 second （秒）部分和 nanosec （纳秒）部分 |
| BaseRealTime | STP 节点启动时的系统真实时间，其中包含 second （秒）部分和 nanosec （纳秒）部分 |
| Offset | STP 节点用于计算相对于 STP server 同步节点的时间偏移，单位为纳秒 |
| SlewRate | STP 节点用于计算相对于 STP server 同步节点的 CPU tick 的比率，单位为 1/10000 |
| TimeError | STP 节点当前的时间容错误差，单位为纳秒 |
| MetaLSN | STP 节点已完成同步的元数据 LSN，其中包含 offset（LSN 的偏移）和 version（当前 LSN 的版本号）信息 |

> **Note:**
>
> * SyncHWTime 和 BaseHWTime 是基于系统的运行时间计算的，与逻辑时间和系统时间无关

**示例**

```lang-bash
$ bin/stpq --meta
Meta:
   MetaSHMKey   : 9622
   Version      : 1
   SyncInterval : 60
   SyncHWTime   : ( 1801227 second, 379397078 nanosec )
   BaseHWTime   : ( 1800687 second, 344618629 nanosec )
   BaseRealTime : ( 1590384465 second, 718350000 nanosec )
   Offset       : 0
   SlewRate     : 10000
   TimeError    : 1000000
   MetaLSN      : ( offset 1590385005753126, version 3 )
```

### 查询 server 信息

--servers 查询 STP 节点进行同步的 server 组的信息

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| Version | STP server 组的版本号 |
| Server | STP server 组的信息，一般格式是 "hostname:port"<br/>当 STP server 组有多个 server 时，将显示多行 |
| Primary | STP server 组的主节点 |

**示例**

```lang-bash
$ bin/stpq --servers
Servers:
   Version : 1
   Server  : sdbserver1:9622
   Server  : sdbserver2:9622
   Server  : sdbserver3:9622
   Primary : sdbserver4:9622
```

### 查询同步客户端信息

--syncclients 查询 STP 节点所在 STP 集群的时间同步信息

> **Note:**
>
> 用户通过任意 STP 节点执行的命令，都将会自动转发至 STP server 主节点上执行。

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| Source | STP 同步源的信息，即 STP server 主节点，一般格式是 "hostname:port" |
| Client | STP 同步节点的信息，一般格式是"hostname:port" |
| Role | STP 同步节点的角色，"server" 或者"client" |
| Port | STP 同步节点使用 STP server 主节点的端口 |
| Status | STP 同步节点的状态 |
| Count | STP 同步节点向 STP server 主节点同步次数 |
| Interval | STP 同步节点的同步间隔，单位为秒 |
| TimeError | STP 同步节点的时间容错误差，单位为微秒，格式为 <当前时间容错误差>/<最大时间容错误差> |
| Passed | STP 同步节点上次同步后经过的时间，单位为微秒 |

> **Note:**
>
> * STP server 备节点也需要向 STP server 主节点进行时间同步
> * STP 的同步状态包括
>    * CheckOffset：快速检查时间偏移
>    * CheckSlewRate：检查节点间 CPU 的频率比例
>    * RecheckOffset：再次快速检查时间偏移
>    * IntervalCheck：周期性检查同步
>    * CheckError：（可能由于网络拥堵引起的）同步出错
>    * NoSource：没有找到同步源
> * STP 同步节点的最大时间容错误差是通过 STP 节点的 maxtimeerror 配置的

**示例**

```lang-bash
$ bin/stpq --syncclients
Synchronize Source: server-3:9622
Synchronize Clients:
   Client          Role   Port Status        Count Interval TimeError  Passed
   sdbserver1:9622 server 9622 IntervalCheck 36    60       1000/50000 45920000
   sdbserver2:9622 server 9622 IntervalCheck 36    60       1000/50000 47100000
   Total: 2
```

### 查询同步信息

--syncstatus 查询 STP 节点和当前同步源的同步信息

> **Note:**
>
> STP server 主节点作为同步源，不需要与任何节点进行同步，所以没有同步状态信息。

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| Role | STP 节点的角色，"server" 或者 "client" |
| Primary | STP 节点是否为 server 主节点 |
| Status | STP 节点的同步状态 |
| Source | STP 节点的同步源，一般为 STP server 主节点，格式为 "hostname:port" |
| Count | STP 节点与同步源的同步次数，格式为 <有效次数>/<总次数> |
| Delay | STP 节点与同步源的延迟，单位为微秒，格式为 <最小延迟>/<最大延迟>/<上次延迟> |
| Offset | STP 节点与同步源的时间偏移，单位为微秒，格式为 [<最小负偏移>,<最大负偏移>]/[<最小正偏移>,<最大正偏移>]/<上次偏移> |
| Passed | STP 节点与同步源上次同同步后经过的时间，单位为微秒 |

同步请求历史信息

| 字段名 | 描述 |
| ---- | ---- |
| RequestID | STP 节点同步请求消息 ID |
| Valid | STP 节点同步请求是否有效（在有效的时间容错误差范围内） |
| Status | STP 节点发送同步请求时的同步状态 |
| Delay | STP 节点同步请求的延迟，单位为微秒 |
| Offset | STP 节点同步请求得到的时间偏移，单位为微秒 |
| Passed | STP 节点同步请求后经过的时间，单位为微秒 |

**示例**

```lang-bash
$ bin/stpq --syncstatus
Synchronize Status:
   Role   Primary Status        Source        Count Delay       Offset               Passed
   server FALSE   IntervalCheck sdbserver1:9622 32/32 144/685/196 [-1,-208]/[2,219]/59 25390000
Synchronize history:
   RequestID Valid Status        Delay Offset               Passed
   27        TRUE  CheckSlewRate 685   -208                 223930000
   29        TRUE  CheckSlewRate 188   219                  214000000
   30        TRUE  CheckSlewRate 317   -31                  204080000
   31        TRUE  CheckSlewRate 347   67                   194150000
   32        TRUE  CheckSlewRate 246   -20                  184220000
   33        TRUE  CheckSlewRate 195   -6                   174290000
   34        TRUE  CheckSlewRate 173   -13                  164370000
   36        TRUE  CheckSlewRate 221   4                    154440000
   37        TRUE  RecheckOffset 168   -5                   153450000
   38        TRUE  RecheckOffset 262   6                    152450000
   39        TRUE  RecheckOffset 194   9                    151460000
   40        TRUE  RecheckOffset 327   -45                  150470000
   41        TRUE  RecheckOffset 408   -86                  149480000
   ...
```

### 查询同步历史

--synchistory 查询 STP 节点和各个同步源的历史时间同步信息

> **Note:**
>
> 同步历史信息每隔两小时会清理一次，用户应根据需要保留必要的信息。

**结果字段**

| 字段名 | 描述 |
| ---- | ---- |
| Source | STP 节点的同步源，一般为 STP server 主节点，格式为 "hostname:port" |
| Count | STP 节点与同步源的同步次数，格式为 <有效次数>/<总次数> |
| Delay | STP 节点与同步源的延迟，单位为微秒，格式为 <最小延迟>/<最大延迟>/<上次延迟> |
| Offset | STP 节点与同步源的时间偏移，单位为微秒，格式为 [<最小负偏移>,<最大负偏移>]/[<最小正偏移>,<最大正偏移>]/<上次偏移> |
| Passed | STP 节点与同步源上次同同步后经过的时间，单位为微秒 |

**示例**

```lang-bash
$ bin/stpq --synchistory
Synchronize History:
   Source        Count Delay       Offset              Passed
   sdbserver1:9622 50/50 193/331/241 [0,-68]/[0,48]/-11  23450000
   sdbserver2:9622 37/38 212/471/247 [0,-112]/[0,137]/10 144480000
   Total: 2
```
