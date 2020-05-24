##语法##
***db.deleteConf( \<config\>, [options] )***

恢复配置默认值，进行配置动态生效，并将配置从配置文件中删除。重启生效的配置需重启后生效，禁止修改的配置则不允许修改。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| config | Json对象 |配置参数，包含配置名和占位符，例如：{ preferedinstance:1, diaglevel:1 }，其中 1 没有特殊含义，仅作为占位符出现。| 是 |
| options| Json对象 | **[命令位置参数](reference/Sequoiadb_command/location.md)** | 否 |

> **Note:**
>
> * 参照[数据库配置](database_management/database_configuration/configuration_parameters.md)页面获取配置的动态生效、重启生效和禁止修改信息。
> * 重启生效和禁止修改配置的详细信息会通过错误信息返回值通知。
> * 若配置的默认值和数据库当前值相同，则重启生效和禁止修改配置不会报错。
> * 无命令位置参数时，缺省对所有节点生效，即使用 {Global:true} 的命令位置参数。
> * 可以通过 [snapshot](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md) 获取指定节点的当前配置。


##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](reference/Sequoiadb_command/Global/getLastErrObj.md)  或 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。
更多错误可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##版本信息##
2.9及以上版本

##示例##

* 删除数据节点 20000 上的 diaglevel 参数，恢复其默认值。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.deleteConf( { diaglevel:1 }, { GroupName:"db1", ServiceName:"20000" } )
 ```

* 删除数据组 db2 上所有数据节点的 preferedinstance 和 diaglevel 参数，恢复默认值。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 > db.deleteConf( { preferedinstance:1, diaglevel:1 }, { GroupName:"db2" } )
 ```

* 报错时获取详细错误信息。

 ```lang-javascript
 // 连接协调节点
 > db = new Sdb( "localhost", 11810 )
 // 进行参数配置，报错
 > db.deleteConf( { transactionon:1 }, { ServiceName:"20000" } )
   (nofile):0 uncaught exception: -264
   One or more nodes did not complete successfully
   Takes 0.009322s.
 // 获取详细报错信息，了解到 transactionon 参数需要重启生效
 > getLastErrObj()
	{
		"errno": -264,
		"description": "One or more nodes did not complete successfully",
		"detail": "",
		"ErrNodes": [
		{
			"NodeName": "ubuntu-zwb:20000",
			"GroupName": "db1",
			"Flag": -322,
			"ErrInfo": {
			"errno": -322,
			"description": "Some configuration changes didn't take effect",
			"detail": "Config 'transactionon' require(s) restart to take effect."
			}
		}
		]
	}
Takes 0.004652s.
 ```
