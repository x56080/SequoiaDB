[^_^]:
    变更流

SequoiaDB 巨杉数据库提供变更流机制（Change Streams），可以订阅数据库中的变更事件。无论是插入新的文档，还是更新、删除已存在的文档，甚至数据模型的改变，如集合的创建和删除，都会被变更流捕捉到。这使得开发者能够实时捕获和响应数据库的变化，使应用程序具有更高的反应速度和数据一致性。变更流提供了强大的实时数据变更监测功能，可以广泛应用在各类场景中，如实时分析、触发器、流式处理等。

## 使用 ##

使用变更流的主要步骤如下：

1. 创建或者获取 StreamToken 对象，它将定义本次变更流的开始的位置。

```lang-javascript
var token = db.getChangeStreamToken();
```

2. 设定订阅选项，这包括需要订阅的数据集合、数据集合空间，变更事件类型，等待的时间，缓存日志的数据量等。

```lang-javascript
var options = { Collections: [ "foo.bar" ], ChangeTypes: "RECORD|DDL", MaxWaitTime: 1, CacheSize: 32 }
```

3. 使用Sdb.watch()函数进行订阅。这将返回一个 SdbCursor 对象。

```lang-javascript
var cursor = db.watch(token, options);
```

4. 通过Cursor对象获取返回的变更流数据。

```lang-javascript
while (cursor.next()) {
    var record = cursor.current();
    println(record);
}
```

## 变更流数据格式 ##

变更流一般会返回两种类型的记录：变更（Change）类型记录和控制（Control）类型记录。

### 控制（Control）类型记录 ###

变更流的控制记录，表示终止、错误等状态。

```lang-javascript
{
   Token: "<token>",
   Type: "control",
   ControlType："<control type>"
   ControlRC：<rc>,
   ControlReason："reason"
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Token | string | 当前记录的读取位置信息（发生错误的位置信息） |
| Type | string | 控制记录类型为 "control" |
| ControlType | string | 控制记录的类型，取值如下：</br>"empty"：当前变更流没有数据</br>"error"：当前变更流发生错误终止 |
| ControlRC | int32 | 控制记录的返回码 |
| ControlReason | string | 控制记录的产生的原因 |

常见的控制记录返回码如下：

| 错误码 | 错误类型 | 可能发生的原因 |
| --- | --- | --- |
| -23 | SDB_DMS_NOTEXIST | 订阅的集合被删除、或者被改名 |
| -34 | SDB_DMS_CS_NOTEXIST | 订阅的集合空间被删除、或者被改名 |
| -24 | SDB_DMS_RECORD_TOO_BIG | 订阅的集合中的记录超过16MB限制 |
| -405 | SDB_STREAM_NOT_CATCHUP | 数据流的读取速度赶不上订阅对象的写入速度 |
| -406 | SDB_DPS_LSN_MOVED | 日志的 LSN 被移动 |

### 变更（Change）类型记录 ###

变更流的变更记录，表示订阅对象的变更事件，一般是从DPS日志中抽取的日志。

```lang-javascript
{
   Token: "<token>",
   Type: "change",
   ChangeType："<change type>",
   ChangeFlags: "<change flags>",
   CollectionSpace: "<collection space name>",
   Collection: "<collection full name>",
   // Desctiption
   ...,
   TransInfo:
   {
      TransID: "<transaction ID>",
      TransAttr: <transaction attributes>
      ...
   },
   TimeInfo:
   {
      RealTime: <real time>
   }
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Token | string | 当前记录的读取位置信息 |
| Type | string | 变更记录类型为 "change" |
| ChangeType | string | 变更记录的类型 |
| ChangeFlags | string | 变更记录的标记</br>如 "NonBusinessOp"，标识内部操作变更，由切分等操作触发等 |
| CollectionSpace | string | 变更记录所属的集合空间 |
| Collection | string | 变更记录所属的集合 |
| Description | bson | 变更记录的具体内容，根据变更记录的类型不同，具体内容也不同 |
| TransInfo | bson | 变更记录的事务信息，如果是非事务操作，该字段为空 |
| TransInfo.TransID | string | 事务ID |
| TransInfo.TransAttr | string | 事务的属性 |
| TimeInfo | bson | 变更记录的时间信息 |
| TimeInfo.RealTime | int64 | 变更操作发生的时间戳，需要开启节点的[配置参数][config] logtimeon |

变更记录的类型说明如下：

| 变更记录类型 | 分类 | 描述 |
| --- | --- | --- |
| ["insert"][insert] | RECORD | 插入记录操作 |
| ["update"][update] | RECORD | 更新记录操作 |
| ["delete"][delete] | RECORD | 删除记录操作 |
| ["truncatecl"][truncatecl] | RECORD、LOB | 清空集合操作 |
| ["lobwrite"][lobwrite] | LOB | 写大对象操作 |
| ["lobupdate"][lobupdate] | LOB | 更新大对象操作 |
| ["lobremove"][lobremove] | LOB | 删除大对象操作 |
| ["createcs"][createcs] | DDL | 创建集合空间操作 |
| ["deletecs"][deletecs] | DDL | 删除集合空间操作 |
| ["renamecs"][renamecs] | DDL | 重命名集合空间操作 |
| ["createcl"][createcl] | DDL | 创建集合操作 |
| ["deletecl"][deletecl] | DDL | 删除集合操作 |
| ["renamecl"][renamecl] | DDL | 重命名集合操作 |
| ["createix"][createix] | DDL | 创建索引操作 |
| ["deleteix"][deleteix] | DDL | 删除索引操作 |
| ["alter"][alter] | DDL | 修改集合空间、集合属性操作 |
| ["return"][return] | DDL | 恢复回收站项目操作 |
| ["invalidatecata"][invalidatecata] | DDL | 清空编目缓存操作 |
| ["commit"][commit] | TRANS | 提交事务操作 |
| ["rollback"][rollback] | TRANS | 回滚事务操作 |

[^_^]:
    本文使用到的所有内部链接及引用
[config]:manual/Manual/Database_Configuration/configuration_parameters.md
[insert]:manual/Distributed_Engine/Maintainance/Change_Stream/insert.md
[update]:manual/Distributed_Engine/Maintainance/Change_Stream/delete.md
[delete]:manual/Distributed_Engine/Maintainance/Change_Stream/delete.md
[truncatecl]:manual/Distributed_Engine/Maintainance/Change_Stream/truncatecl.md
[lobwrite]:manual/Distributed_Engine/Maintainance/Change_Stream/lobwrite.md
[lobupdate]:manual/Distributed_Engine/Maintainance/Change_Stream/lobupdate.md
[lobremove]:manual/Distributed_Engine/Maintainance/Change_Stream/lobremove.md
[createcs]:manual/Distributed_Engine/Maintainance/Change_Stream/createcs.md
[deletecs]:manual/Distributed_Engine/Maintainance/Change_Stream/deletecs.md
[renamecs]:manual/Distributed_Engine/Maintainance/Change_Stream/renamecs.md
[createcl]:manual/Distributed_Engine/Maintainance/Change_Stream/createcl.md
[deletecl]:manual/Distributed_Engine/Maintainance/Change_Stream/deletecl.md
[renamecl]:manual/Distributed_Engine/Maintainance/Change_Stream/renamecl.md
[createix]:manual/Distributed_Engine/Maintainance/Change_Stream/createix.md
[deleteix]:manual/Distributed_Engine/Maintainance/Change_Stream/deleteix.md
[alter]:manual/Distributed_Engine/Maintainance/Change_Stream/alter.md
[return]:manual/Distributed_Engine/Maintainance/Change_Stream/return.md
[invalidatecata]:manual/Distributed_Engine/Maintainance/Change_Stream/invalidatecata.md
[commit]:manual/Distributed_Engine/Maintainance/Change_Stream/commit.md
[rollback]:manual/Distributed_Engine/Maintainance/Change_Stream/rollback.md