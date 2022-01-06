stp 是 STP 提供逻辑时间的可执行程序。

## 参数说明

| 参数名 | 缩写 | 类型 | 说明 |
| ------ | ---- | ---- | ---- |
| --help | -h | | 返回 `stp` 的用法和帮助 |
| --version | | | 返回 `stp` 的版本信息 |
| --port | -p | int32 | - STP 监听端口<br/>- 默认：9622<br/>- 开启 TCP 和 UDP 协议的监听 |
| --serverlist | | string | - STP 配置 server 列表，配置后将向指定的 server 进行时间同步<br>- server 的格式为 "hostname:port"，多个 server 之间通过 "," 分隔<br>- 默认：空，表示以本节点作为 server |
| --role | | string | - STP 节点的角色<br/>- 可选值为 "client" 和 "server"<br/>- 默认："server" |
| --syncinterval | | int32 | - STP 节点进行时间同步的间隔，单位为秒<br/>- 默认：60<br/>- 最小值为 10，最大值为 600 |
| --maxtimeerror | | int32 | - STP 节点可以容忍的最大时间误差，单位为微秒</br>- 默认：50000</br>- 最小值为 1000，最大值为 10000000 |
| --diaglevel | | int32 | - STP 节点打印诊断日志的级别<br/>- STP 诊断日志从 0 - 5 分别代表：SEVERE, ERROR, EVENT, WARNING, INFO, DEBUG<br/>- 默认：3，表示 WARNING |
| --daemon | | | 使用后台模式运行 STP 节点 |
| --confpath | -c | string | 指定 STP 的配置目录 |

> **Note:**
>
> * STP 的 "server" 角色
>      * "server" 节点可以用于同步时间的节点，server 之间选举产生主 server 节点，生成全局逻辑时间
>      * STP 最多可以配置 7 个 "server" 角色的节点，因此 serverlist 最多可以配置 7 个节点
> * STP 的 "client" 角色
>      * client 节点只能向 server 节点进行同步
> * `maxtimeerror` 指定的可以容忍的最大时间误差，是指当前 STP 节点与 server 主节点之间的时间误差，详细可参考[逻辑时间][logicaltime]

## 配置参数

`stp` 的参数可以通过在 `安装目录/conf/stp/stp.conf` 中进行配置

| 参数名 | 类型 | 生效类型 | 说明 |
| ------ | ---- | ---- | ---- |
| port | int32 | 禁止修改 | - STP 监听端口<br/>- 默认：9622<br/>- 开启 TCP 和 UDP 协议的监听 |
| serverlist | string | 动态生效 | - STP 配置 server 列表，配置后将向指定的 server 进行时间同步<br>- server 的格式为 "hostname:port"，多个 server 之间通过 "," 分隔<br>- 默认：空，表示以本节点作为 server |
| role | string | 动态生效 | - STP 节点的角色<br/>- 可选值为 "client" 和 "server"<br/>- 默认："server" |
| syncinterval | int32 | 动态生效 | - STP 节点进行时间同步的间隔，单位为秒<br/>- 默认：60<br/>- 最小值为 10，最大值为 600 |
| maxtimeerror | int32 | 动态生效 | - STP 节点可以容忍的最大时间误差，单位为微秒</br>- 默认：50000</br>- 最小值为 1000，最大值为 10000000 |
| diaglevel | int32 | 动态生效 | - STP 节点打印诊断日志的级别<br/>- STP 诊断日志从 0 - 5 分别代表：SEVERE, ERROR, EVENT, WARNING, INFO, DEBUG<br/>- 默认：3，表示 WARNING |

> **Note:**
>
> STP 的配置参数可以通过 [stp.updateConf()][updateConf] 修改，部分参数可以动态生效。

## 后台模式

通过 `daemon` 可以使用后台模式运行 STP 节点，其功能与 [stpstart][start] 相同

```lang-bash
$ bin/stp --daemon
```

## 配置示例

STP 的配置可以分为多 server 模式和单 server 模式

- 多个 "server" 的配置，可以提高 server 的可用性
- 单个 "server" 的配置，使用于1-3个节点较小的集群

**多 server 模式**

选择 3 个 server 节点，sdbserver1:9622、sdbserver2:9622 和 sdbserver3:9622，其余节地作为 client 节点

server 节点的配置如下

```lang-ini
serverlist=sdbserver1:9622,sdbserver2:9622,sdbserver3:9622
role=server
```

client 节点的配置如下

```lang-ini
serverlist=sdbserver1:9622,sdbserver2:9622,sdbserver3:9622
role=client
```

**单 server 模式**

选择 1 个 server 节点，sdbserver1:9622，其余节点作为 client 节点

server 节点的配置如下

```lang-ini
serverlist=sdbserver1:9622
role=server
```

client 节点的配置如下

```lang-ini
serverlist=sdbserver1:9622
role=client
```


[^_^]:
    本文使用的所有引用及链接
[logicaltime]:manual/Distributed_Engine/Architecture/Stp/logicaltime.md
[start]:manual/Distributed_Engine/Architecture/Stp/Tools/stpstart.md
[updateConf]:manual/Manual/Sequoiadb_Command/Stp/updateConf.md
