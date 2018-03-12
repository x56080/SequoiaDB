事务操作。

##语法##

开启事务： ***begin transaction***

提交事务： ***commit***

回滚事务： ***rollback***

##参数##
无

##返回值##
无。

## 示例##

开启事务，插入记录，提交事务。

```lang-javascript
> db.execUpdate( "begin transaction" )
Takes 0.3107s.
> db.execUpdate( "insert into foo.bar(name) values(\"Tom\")" )
Takes 0.3104s.
> db.execUpdate( "commit" )
Takes 0.2100s.
```

开启事务，插入记录，回滚事务。

```lang-javascript
> db.execUpdate( "begin transaction" )
Takes 0.3107s.
> db.execUpdate( "insert into foo.bar(name) values('Tom')" )
Takes 0.3104s.
> db.execUpdate( "rollback" )
Takes 0.1107s.
```