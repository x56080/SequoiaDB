SequoiaDB 巨杉数据库可以通过 SQL 语句进行数据操作，支持的语句如下：

| 语句 | 描述 | 示例 |
|------|------|------|
|create collectionspace| 创建数据库中的集合空间 | db.execUpdate("create collectionspace sample") |
|drop collectionspace| 删除数据库中指定的集合空间 | db.execUpdate("drop collectionspace sample") | 
|create collection| 创建集合 | db.execUpdate("create collection sample.employee")|
|drop collection| 删除集合 | db.execUpdate("drop collection sample.employee") |
|create index| 创建索引 | db.execUpdate("create index test_index1 on sample.employee (age)") |
|drop index| 删除索引 | db.execUpdate("drop index test_index on sample.employee")|
|list collectionspaces| 枚举数据库中所有的集合空间 | db.exec("list collectionspaces") |
|list collections| 枚举数据库中所有的集合 | db.exec("list collections") |
|insert into| 向集合中插入数据 | db.execUpdate("insert into sample.employee(age,name) values(25,"Tom")") |
|select| 查询数据 | db.exec( "select age,name from sample.employee" ) |
|update| 更新数据 | db.execUpdate( "update sample.employee set age=20" ) |
|delete| 删除数据 | db.execUpdate("delete from sample.employee") |
|transaction| 事务操作 | db.execUpdate( "begin transaction" ) |