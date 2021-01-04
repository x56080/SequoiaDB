
##NAME##

listNodes - Display node information.

##SYNOPSIS##

***Sdbtool.listNodes( [options], [filter], [rootPath] )***

##CATEGORY##

Sdbtool

##DESCRIPTION##

Display node information.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| options  | JSON     | display information about data nodes, coordination nodes and catalog nodes by default | display specified type node | not |
| filter   | JSON     | display all information by default | Filtered conditions | not |
| rootPath | JSON     | system configuration filepath | specify the configuration file path | not |

The detail description of 'options' parameter is as follow:

| Attributes | Type | Default | Format | Description |
| ---------- | ---- | ------- | ------ | ----------- |
| type       | string | db  | { type: "all" }<br>{ type: "db" }<br>{ type: "om" }<br>{ type: "cm" } | display information about all nodes <br>display information about data nodes, coordination nodes and catalog nodes<br>display infomation about om node<br>display infomation about cm node |
| mode       | string | run | { mode: "run" }<br>{ mode: "local" } | display infomation about running nodes<br>display infomation about local nodes whether run or not | 
| role       | string | --- | { role: "data" }<br>{ role: "coord" }<br>{ role: "catalog" }<br>{ role: "standalone" }<br>{ role: "om" }<br>{ role: "cm" } | display infomation about data nodes<br>display infomation about coord nodes<br>display infomation about catalog nodes<br>display infomation about standalone nodes<br>display infomation about om node<br>display infomation about cm node |
| svcname    | string | --- | { svcname: "11790" } | display node information of the specified port | 
| showalone  | bool   | false | { showalone: true }<br>{ showalone: false } | whether to diplay information about the cm node started in standalone mode |
| expand     | bool   | false | { expand: true }<br>{ expand: false } | whether to display detailed extended configuration |

>Note:

>1. Cm node has a standalone startup mode. In addition to the current cm node, you can also start a cm node as a temporary cm node in standalone mode(start the cm node to specify the standalone parameter) and the cm node's default survival time is 5 minutes. 

>2. When specifying multiple svcnames, you can separate the svcnames with ",".

>3. The optional parameter filterObj supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return rearch result set.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/faq.md).

##EXAMPLES##

* Display node information.

```lang-javascript
> Sdbtool.listNodes( { type: "all", mode: "local", role: "data", svcname: "20000, 40000" } )
{
  "svcname": "20000",
  "type": "sequoiadb",
  "role": "data",
  "pid": 17390,
  "groupid": 1000,
  "nodeid": 1000,
  "primary": 1,
  "isalone": 0,
  "groupname": "db1",
  "starttime": "2019-05-31-17.14.14",
  "dbpath": "/opt/trunk/database/20000/"
}
{
  "svcname": "40000",
  "type": "sequoiadb",
  "role": "data",
  "pid": 17399,
  "groupid": 1001,
  "nodeid": 1001,
  "primary": 0,
  "isalone": 0,
  "groupname": "db2",
  "starttime": "2019-05-31-17.14.14",
  "dbpath": "/opt/trunk/database/40000/"
}
```

* Display node information and filter the result set

```lang-javascript
> Sdbtool.listNodes( { type: "all", mode: "local", role: "data", svcname: "20000, 40000" }, { groupname: "db2" } )
{
  "svcname": "40000",
  "type": "sequoiadb",
  "role": "data",
  "pid": 17399,
  "groupid": 1001,
  "nodeid": 1001,
  "primary": 0,
  "isalone": 0,
  "groupname": "db2",
  "starttime": "2019-05-31-17.14.14",
  "dbpath": "/opt/trunk/database/40000/"
}
```