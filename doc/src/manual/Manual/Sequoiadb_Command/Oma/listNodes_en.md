
##NAME##

listNodes - Lists all nodes in the host where the current Oma object is connected to.

##SYNOPSIS##

**oma.listNodes(\<options\>,[filter])**

##CATEGORY##

Oma

##DESCRIPTION##

Lists all nodes in the host where the current Oma object is connected to.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| options  | JSON     | Display information about data nodes, coordination nodes and catalog nodes by default | Display specified type node | not |
| filter   | JSON     | Display all information by default | Filtered conditions | not |

The detail description of 'options' parameter is as follows:

| Attributes | Type | Default | Format | Description |
| ---------- | ---- | ------- | ------ | ----------- |
| type       | String | db  | { type: "all" }<br>{ type: "db" }<br>{ type: "om" }<br>{ type: "cm" } | Display information about all nodes <br>Display information about data nodes, coordination nodes and catalog nodes<br>Display infomation about om node<br>Display infomation about cm node |
| mode       | String | run | { mode: "run" }<br>{ mode: "local" } | Display infomation about running nodes<br>Display infomation about local nodes whether run or not | 
| role       | String | --- | { role: "data" }<br>{ role: "coord" }<br>{ role: "catalog" }<br>{ role: "standalone" }<br>{ role: "om" }<br>{ role: "cm" } | Display infomation about data nodes<br>Display infomation about coord nodes<br>Display infomation about catalog nodes<br>Display infomation about standalone nodes<br>Display infomation about om node<br>Display infomation about cm node |
| svcname    | String | --- | { svcname: "11790" } | Display node information of the specified port | 
| showalone  | Bool   | false | { showalone: true }<br>{ showalone: false } | Whether to diplay information about the cm node started in standalone mode |
| expand     | Bool   | false | { expand: true }<br>{ expand: false } | Whether to display detailed extended configuration |

>Note:

>1. Cm node has a standalone startup mode. In addition to the current cm node, you can also start a cm node as a temporary cm node in standalone mode(start the cm node to specify the standalone parameter) and the cm node's default survival time is 5 minutes. 

>2. When specifying multiple svcnames, you can separate the svcnames with ",".

>3. The optional parameter filterObj supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return rearch result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more detial, please  reference to [Troubleshooting](manual/faq.md).

##EXAMPLES##

* Display the 11820 node information. 

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
    > oma.listNodes( { "svcname": '11820'} )
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