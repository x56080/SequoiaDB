SequoiaDB 巨杉数据库审计日志记录了用户对数据库执行的所有操作。通过审计日志，用户可以对数据库进行故障分析、行为分析和安全审计等操作，能有效帮助用户获取数据库的执行情况。

同时，SequoiaDB 提供包含节点级和用户级审计日志，用户可以通过配置决定不同级别的审计日志，或者同一级别不同角色的审计日志。

> **Note：**
> 
>  SequoiaDB 集群模式支持所有级别的审计日志，独立模式仅支持节点级审计日志。

操作类型掩码
----

SequoiaDB 开启或关闭审计日志需要配置操作类型掩码。用户开启审计日志后的每次操作，数据库会根据用户配置的审计日志操作类型掩码输出对应的审计日志，可配置的审计日志操作类型掩码如下：

| 操作类型掩码 | 操作类型                                                 |
| -------- | ------------------------------------------------------------ |
| ACCESS   | 登入登出                                                     |
| CLUSTER  | 集群操作，支持的操作：创建组、删除组                         |
| SYSTEM   | 系统操作，支持的操作：重启节点                               |
| DCL      | 用户操作，支持的操作：创建用户、修改用户、删除用户           |
| DDL      | 集合空间和集合操作，支持的操作：创建集合空间、修改集合空间、删除集合空间、创建集合、修改集合及删除集合 |
| DML      | 数据操作，支持的操作：插入数据、更新数据和删除数据           |
| DQL      | 查询数据                                                     |
| INSERT   | 插入数据                                                     |
| UPDATE   | 更新数据                                                     |
| DELETE   | 删除数据                                                     |
| OTHER    | 其他，以上类型之外的操作                                     |

> **Note：**
>
> - 支持配置"ALL"和"NONE"，配置"ALL"支持所有操作类型的审计日志，配置"NONE"则禁止所有类型的审计日志
> - 支持使用"|"连接多个操作类型，如"DDL|DML|DQL"
> - 支持使用"!"禁止某个操作类型，如"!DCL|DML"；此设置仅允许用户级配置使用
> - 修改配置生效后可执行 [invalidateCache()][invalidateCache] 命令清除 NODE/CATALOG/AUTH 缓存

审计日志优先级：节点 -> 用户，级别关系从大到小。如果用户只开启了某一级别的审计日志，则最终只生效该级别的配置。如果同时开启了多个级别的审计日志，当前级别未配置的操作类型则使用上一级别的配置，以此类推；反之，当前级别配置了但是上一级别未配置，只生效当前级别配置。

**示例**

配置审计日志的操作类型见"配置"列，配置后的生效值见"生效配置"列

```
日志类型       配置              生效配置              说明                        
节点        SYSTEM|ACCESS     SYSTEM|ACCESS    最高级别，只生效当前配置     
用户        !SYSTEM|DCL       DCL|ACCESS       当前级别未配置的使用上级配置                    
```

开启审计日志
----

用户可以通过修改配置的方式开启审计日志。

### 节点级

  - 节点级审计日志默认开启，默认配置的操作类型为"SYSTEM|DDL|DCL"

  - 执行 [updateConf()][updateConf] 命令修改"auditmask"取值并动态生效

    > **Note：**
    >
    > SequoiaDB 巨杉数据库3.0及以上版本配置节点"auditmask"后在线动态生效，v3.0 以下版本需要重启节点生效，详情可参考"[配置项参数](database_management/database_configuration/configuration_parameters.md)"章节 auditmask 参数说明。

**示例** 

修改复制组 db1 上的数据节点 20000，修改 auditmask 取值为"SYSTEM|DDL|DCL"

```
> db.updateConf( { auditmask:"SYSTEM|DDL|DCL"}, { GroupName:"db1", ServiceName:"20000"} ) 
```

### 用户级

 新用户执行 [createUsr()][createUsr] 命令配置 AuditMask，并重新登录用户使配置生效

   **示例**
    
   创建 `admin` 用户，配置用户名为"admin"，密码为"admin"，操作类型掩码 AuditMask 为"DDL|DML|!DQL"

   ```
   > db.createUsr( "admin", "admin", { AuditMask: "DDL|DML|!DQL" } )
   ```

查看审计日志配置
----

### 节点级

用户在数据库安装目录下执行 `bin/sdblist --detail --expand` 可以查看 auditmask 取值。

**示例**

- 查看 coord 节点的审计日志配置信息，sdblist 命令详解可参考`bin/sdblist --help`帮助信息

   ```lang-bash
   $ ./bin/sdblist --detail --expand -r coord
   ```
    
- 该示例只展示审计日志相关配置。auditpath 表示审计日志文件路径，默认为 `数据文件路径/diaglog/sdbaudit.log`；auditnum 表示审计日志文件个数，默认为 20；auditmask 表示审计日志操作类型掩码，默认为"SYSTEM|DDL|DCL"。其他配置以省略号“......”表示，返回结果如下：

   ```lang-text
   ......
   auditpath      :  /opt/sequoiadb/database/coord/11810/diaglog/
   auditnum       :  20
   auditmask      :  SYSTEM|DDL|DCL
   ......
   ```

### 用户级

用户可以通过[用户列表][SDB_LIST_USERS]查看指定用户的 AuditMask 取值。

**示例**

查看 sample 用户的操作类型掩码 AuditMask 配置

```lang-javascript
> db.list( SDB_LIST_USERS )
{
    "User": "sample"
    "Options": {
        "AuditMask": "DDL|DML|!DQL"
    }
}
```

查看审计日志信息
----

SequoiaDB 只能查看节点路径下的审计日志文件。如果审计日志文件路径非默认路径，可以在数据库安装目录下执行 `bin/sdblist --detail` 查看 auditpath，再查看对应路径下的审计日志文件。

`sdbaudit.log` 审计日志内容包含如下字段，说明如下：

| 字段       | 说明                                                |
| ---------- | --------------------------------------------------- |
| Type       | 操作类型，如"DML"                                   |
| PID        | 进程号                                              |
| TID        | 线程号                                              |
| UserName   | 操作用户，如"admin"                                 |
| From       | 操作地址，如"192.168.11.11:88888"                   |
| Action     | 操作，如"INSERT"                                    |
| Result     | 操作结果，如"SUCCEED(0)" ，其中括号内的数字为操作结果返回码|
| ObjectType | 对象类型，如"COLLECTION"                            |
| ObjectName | 对象名，如"sample.employee"                         |
| Message    | 详细信息                                            |

> **Note：**
>
> Result 操作结果中的返回码，0 表示操作成功，非 0 表示操作失败（如 -33 创建集合失败）。非0返回码对应解读信息可参考[错误码](reference/Sequoiadb_error_code.md)。

**示例**

在 SequoiaDB 创建集合 sample.employee，查看当前 coord 节点数据目录下的 `sdbaudit.log` 审计日志文件，内容如下：

```
2018-08-24-17.45.49.444138               Type:DDL
PID:7479                                 TID:5011
UserName:                                From:192.168.3.24:32974
Action:COMMAND(create collection)        Result:SUCCEED(0)
ObjectType:COLLECTION                    ObjectName:sample.employee
Message:
Option: { "Name": "sample.employee" }
```

关闭审计日志
----

### 节点级

关闭审计日志的方法同开启审计日志，执行 [updateConf()][updateConf] 命令修改 auditMask 取值为"NONE"并动态生效，生效后将不再输出对应级别的审计日志。

### 用户级

用户可以通过 [dropUsr()][dropUsr] 删除指定用户来关闭审计日志。

**示例**

删除用户名为"admin"，密码为"admin"的数据库权限

```lang-javascript
db.dropUsr( "admin", "admin" )
```


[^_^]:
     本文使用的所有链接和引用
[invalidateCache]:manual/reference/Sequoiadb_command/Sdb/invalidateCache.md
[updateConf]:manual/reference/Sequoiadb_command/Sdb/updateConf.md
[createUsr]:manual/reference/Sequoiadb_command/Sdb/createUsr.md
[SDB_LIST_USERS]:manual/Maintainance/Monitoring/list/SDB_LIST_USERS.md
[dropUsr]:manual/reference/Sequoiadb_command/Sdb/dropUsr.md