事务是由一系列操作组成的逻辑工作单元。在同一个会话（或连接）中，同一时刻只允许存在一个事务，也就是说当用户在一次会话中创建了一个事务，在这个事务结束前用户不能再创建新的事务。

事务作为一个完整的工作单元执行，事务中的操作要么全部执行成功要么全部执行失败。SequoiaDB事务中的操作只能是插入数据、修改数据以及删除数据，在事务过程中执行的其它操作不会纳入事务范畴，也就是说事务回滚时非事务操作不会被执行回滚。如果一个表或表空间中有数据涉及事务操作，则该表或表空间不允许被删除。

默认情况下，事务功能是关闭的。

如要打开事务功能需要在节点的配置文件中配置参数：transactionon = TRUE；在创建数据节点时，增加 JSON 类型的参数：{ "transactionon": "YES" } 或 { "transactionon": true }。

注意：要打开事务功能，必须将[logfilenum](database_management/runtime_configuration.md)设置为大于等于5的值（如果未单独配置，其默认为20，则不需要修改）

## 示例##

* 开启事务：

```lang-javascript
> db.transBegin()
```

* 插入记录：

```lang-javascript
> cl.insert( { date: 99, id: 8, a: 0 } )
```

* 回滚事务，插入的记录将被回滚，集合中无记录：

```lang-javascript
> db.transRollback()
> cl.count()
Return 0 row(s). 
```

* 开启事务：

```lang-javascript
> db.transBegin()
```

* 插入记录：

```lang-javascript
> cl.insert( { date: 99, id: 8, a: 0 } )
```

* 提交事务，插入的记录将被写入数据库：

```lang-javascript
> db.transCommit()
> cl.count()
Return 1 row(s). 
```