
##名称##

listNodes - 列出当前所连 sdbcm 所在机器符合条件的所有节点的信息。

##语法##

**oma.listNodes(\[options\],[filter])**

##类别##

Oma

##描述##

列出当前所连 sdbcm 所在机器符合条件的所有节点的信息。

##参数##

| 参数名   | 参数类型 | 默认值               | 描述                | 是否必填 |
| -------- | -------- | -------------------- | ------------------- | -------- |
| options  | JSON     | 默认显示数据节点，协调节点和编目节点的信息 | 显示指定类型的节点的信息 | 否 |
| filter   | JSON     | 默认显示全部内容     | 筛选条件            | 否       |

options 参数详细说明如下：

| 属性 | 值类型 | 默认值 | 格式 | 描述 |
| ---- | ------ | ------ | ---- | ---- |
| type | String |  db |{ type: "all" }<br>{ type: "db" }<br>{ type: "om" }<br>{ type: "cm" } |  显示所有节点的信息<br>显示数据节点，协调节点和编目节点的信息<br>显示 om 节点的信息<br>显示 cm 节点的信息 |
| mode | String | run |{ mode: "run" }<br>{ mode: "local" } | 显示正在运行的节点的信息<br>显示本地节点的信息，无论是否正在运行 |
| role      | String | 空 | { role: "data" }<br>{ role: "coord" }<br>{ role: "catalog" }<br>{ role: "standalone" }<br>{ role: "om" }<br>{ role: "cm" } | 显示数据节点的信息<br>显示协调节点的信息<br>显示编目节点的信息<br>显示 standalone 节点的信息<br>显示 om 节点的信息<br>显示 cm 节点的信息 |
| svcname   | String | 空 | { svcname: "11790" } | 显示指定端口节点的信息 |
| showalone | Bool | false | { showalone：true }<br>{ showalone: false } | 是否显示以 standalone 模式启动的 cm 节点的信息 |
| expand    | Bool | false | { expand: true }<br>{ expand: false } | 是否显示详细的扩展配置 |

> Note：

> 1. cm 有 standalone 的启动模式。除了当前的 cm 之外，还可以通过 standalone 模式再启动一个 cm 作为临时 cm (启动 cm 的时候指定 standalone 参数)，默认存活时间为 5 分钟。

> 2. 当指定多个 svcname 时，可以以 ‘,’ 隔开。

> 3. filter 参数支持对结果中的某些字段进行 and 、 or 、not 和精确匹配计算，对结果集进行筛选。

##返回值##

成功：返回 sdbcm 所在机器符合条件的所有节点信息。  

失败：无。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0及以上版本。

##示例##

1. 连接到本地的集群管理服务进程 sdbcm，获取 11820 节点的信息。

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
    > oma1.listNodes( { "svcname": '11820'} )
    {
      "svcname": "11820",
      "type": "sequoiadb",
      "role": "data",
      "pid": 23240,
      "groupid": 1000,
      "nodeid": 1000,
      "primary": 0,
      "isalone": 0,
      "groupname": "group1",
      "starttime": "2010-02-05-15.42.00",
      "dbpath": "/opt/sequoiadb/database/data/11820/"
    }
	```