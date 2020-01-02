事务是由一系列操作组成的逻辑工作单元。在同一个会话（或连接）中，同一时刻只允许存在一个事务，也就是说当用户在一次会话中创建了一个事务，在这个事务结束前用户不能再创建新的事务。

事务作为一个完整的工作单元执行，事务中的操作要么全部执行成功要么全部执行失败。SequoiaDB事务中的操作只能是插入数据、修改数据以及删除数据，在事务过程中执行的其它操作不会纳入事务范畴，也就是说事务回滚时非事务操作不会被执行回滚。如果一个表或表空间中有数据涉及事务操作，则该表或表空间不允许被删除。

## 事务启停 ##

[数据库配置](database_management/database_configuration/configuration_parameters.md)中，关于事务启停的配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| ------ | ------ | --- | ------ |
| transactionon | 表示 SequoiaDB 是否开启事务功能。 | true/false | true |

默认情况下，SequoiaDB 所有节点的事务功能都是开启的。若用户不需要使用事务功能，可参考示例 3 **全局关闭事务** 的方法，关闭事务功能。

注意：

1. 开启及关闭节点的事务功能都要求重启该节点。
2. 在开启节点事务功能的情况下，节点的配置项[logfilenum](database_management/database_configuration/configuration_parameters.md)（该配置项默认值为20）的值不能小于 5。

## 事务操作 ##

SequoiaDB 事务支持的操作如下：

* 写事务操作：INSERT、UPDATE、DELETE。
* 读事务操作：QUERY。

SequoiaDB的其它操作（如：创建表、创建索引、创建并读写LOB等其它非 CRUD 操作）不在事务功能的考虑范围。

## 隔离级别 ##

[数据库配置](database_management/database_configuration/configuration_parameters.md)中，关于事务隔离级别的配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| ------ | ------ | --- | ------ |
| transisolation | 表示在开启事务的情况下，使用的事务隔离级别。 | 0 表示 RU，1 表示 RC，2 表示 RS | 0 |

## 事务开启、提交与回滚 ##

通过 "transBegin"、"transCommit" 及 "transRollback" 方法，用户可以在一个事务中，对若干个操作进行事务控制。其使用方式如下：

```lang-javascript
> db.transBegin()
> 操作1
> 操作2
> 操作3
> ...
> db.transCommit() or db.transRollback()
```
在上述使用模式中，用户必须显式调用 "transCommit" 及 "transRollback" 方法来结束当前事务。  
然而，对于写事务操作，若在操作过程发生错误，[数据库配置](database_management/database_configuration/configuration_parameters.md)中的 transautorollback 配置项可以决定当前会话所有未提交的写操作是否自动回滚。transautorollback 的描述如下：

| 配置项 | 描述 | 取值 | 默认值 |
| ------ | ------ | --- | ------ |
| transautorollback | 表示 **写事务操作** 失败时，是否自动回滚。 | true/false | true |
**注意**：该配置项只有在事务功能开启（即 transactionon 为 true ）的情况下才生效。

默认情况下，transautorollback 配置项的值为 true。所以，当写事务操作过程出现失败时，当前事务所有未提交的写操作都将被自动回滚。

## 事务自动提交 ##

[数据库配置](database_management/database_configuration/configuration_parameters.md)中，关于事务自动提交的配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| ------ | ------ | --- | ------ |
| transautocommit | 表示是否开启事务自动提交功能。 | true/false | false |

**注意**：该配置项只有在事务功能开启（即 transactionon 为 true ）的情况下才生效。

事务自动提交功能默认情况下是关闭的。当 transautocommit 设置为 true 时，事务自动提交功能将开启。此时，使用事务存在以下两点不同：

* 用户不需要显式调用 "transBegin" 和 "transCommit" 或者 "transRollback" 方法来控制事务的开启、提交或者回滚。
* 事务提交或者回滚的范围仅仅局限于单个操作。当单个操作成功时，该操作将被自动提交；当单个操作失败时，该操作将被自动回滚。

	例如，如下操作中：

	```lang-javascript
	> /* transautocommit 设置为 true */
	> db.foo.bar.update({$inc:{"salary": 1000}}, {"department": "A"}) // 更新 1
	> db.foo.bar.update({$inc:{"salary": 2000}}, {"department": "B"}) // 更新 2
	> db.foo.bar.update({$inc:{"salary": 3000}}, {"department": "C"}) // 更新 3
	> ...
	```

	更新 1、更新 2、更新 3 分别为独立的操作。假设更新 1 和 更新 2 操作成功，而更新 3 失败。那么更新 1 和 更新 2 修改的记录将全部被自动提交。而更新 3 修改的记录将全部被自动回滚。

## 其它配置 ##

[数据库配置](database_management/database_configuration/configuration_parameters.md)中，关于事务的其它主要配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| ------ | ------ | --- | ------ |
| transactiontimeout | 事务锁等待超时时间（单位：秒） | [0, 3600] | 60 |
| translockwait | 事务在RC隔离级别下是否需要等锁。 | true/false | false |
| transuserbs | 事务操作是否使用回滚段。 | true/false | true |

## 调整设置 ##

当用户希望调整事务的设置时（如：是否开启事务、调整事务配置项等），有如下 3 种方式供用户选择使用：

1. 修改节点配置文件

	用户可以将[数据库配置](database_management/database_configuration/configuration_parameters.md)描述的事务配置项，配置到集群所有（或者部分）节点的配置文件中。若修改的配置项要求重启节点才能生效，用户需重启相应的节点。
2. 使用 [updateConf()](reference/Sequoiadb_command/Sdb/updateConf.md) 命令在 sdb shell 中修改集群的事务配置项。若修改的配置项要求重启节点才能生效，用户需重启相应的节点。
3. 使用 [setSessionAttr()](reference/Sequoiadb_command/Sdb/setSessionAttr.md) 命令在会话中修改当前会话的事务配置项。该设置只在当前会话生效，并不影响其它会话的设置情况。


## 示例 ##

假设集群的安装目录为 "/opt/sequoiadb"，协调节点地址为 "ubuntu-dev1:11810"。通过如下操作，获取 db 以及 cl 对象。

```lang-javascript
> db = new Sdb( "ubuntu-dev1", 11810 )
> cl = db.createCS("foo").createCL("bar")
```

1. 使用事务回滚插入操作。事务回滚后，插入的记录将被回滚，集合中无记录：

	```lang-javascript
	> cl.count()
	Return 0 row(s). 
	> db.transBegin()
    > cl.insert( { date: 99, id: 8, a: 0 } )
	> db.transRollback()
	> cl.count()
	Return 0 row(s). 
	```

2. 使用事务提交插入操作。提交事务后，插入的记录将被持久化到数据库：

	```lang-javascript
	> cl.count()
	Return 0 row(s). 
	> db.transBegin()
    > cl.insert( { date: 99, id: 8, a: 0 } )
	> db.transCommit()
	> cl.count()
	Return 1 row(s). 
	```

3. 全局关闭事务。

	步骤1：通过sdb shell 设置集群所有节点都关闭事务。

	```lang-javascript
	> db.updateConf( { transactionon: false }, { Global: true } )
	```

	步骤2：在集群每台服务器上都重启 SequoiaDB 的所有节点。

	```lang-bash
	[sdbadmin@ubuntu-dev1 ~]$ /opt/sequoiadb/bin/sdbstop -t all
	[sdbadmin@ubuntu-dev1 ~]$ /opt/sequoiadb/bin/sdbstart -t all
	```
	注意：必须在每台服务器上都重启 SequoiaDB 的所有节点，才能保证事务功能在所有节点上都关闭的。