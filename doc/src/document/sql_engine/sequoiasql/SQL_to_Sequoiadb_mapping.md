##概念和术语##

| SQL                                      | SequoiaDB                |
| ---------------------------------------- | ------------------------ |
| database                                 | collection space         |
| table                                    | collection               |
| row                                      | document / BSON document |
| column                                   | field                    |
| index                                    | index                    |
| primary key （指定任何唯一的列作为主键） | primary key （是自动创建到记录的 \_id 字段） |

##Create 和 Alter##

下表列出了各种表级别的操作：

| SQL 语句 | SequoiaDB 语句 |
| -------- | -------------- |
| CREATE TABLE bar ( name char(10), age  integer ); | db.foo.createCL( "bar" ) |
| ALTER TABLE bar ADD COLUMN sex char(5); | 集合不强制执行文档的结构，即在集合上不需要改动结构操作 |
| ALTER TABLE bar DROP COLUMN sex; |  集合不强制执行文档的结构，即在集合上不需要改动结构操作 |
| CREATE INDEX aIndex ON bar (age); | db.foo.bar.createIndex( "aIndex", { age: 1 } )  | 
| DROP TABLE bar; | db.foo.dropCL( "bar" ) |

##Insert##

下表列出了在表上的插入操作：

| SQL 语句                                        | SequoiaDB 语句   |
|------------------------------------------------ | -----------------|
| INSERT INTO bar VALUES ('Harry',8); | db.foo.bar.insert( { name: "Harry", age: 8 } ) |

##Select##

下表列出了在表上的读操作：

| SQL 语句                        | SequoiaDB 语句                                 |
|-------------------------------- |----------------------------------------------- |
| SELECT * FROM bar;              | db.foo.bar.find() |
| SELECT name, age FROM bar;      | db.foo.bar.find( {},{ name: null, age: null } ) |
| SELECT * FROM bar WHERE age > 25; | db.foo.bar.find( { age: { $gt: 25 } } )            |
| SELECT age FROM bar WHERE age = 25 AND name = 'Harry'; | db.foo.bar.find( { age: 25, name: "Harry" }, { age: null } ) |
| SELECT COUNT( * ) FROM bar;    | db.foo.bar.COUNT()                             |
| SELECT COUNT( name ) FROM bar;| db.foo.bar.COUNT( { name: { $exists: 1 } } )     |


##Update##

下表列出了在表上的更新操作：

| SQL 语句                                             | SequoiaDB 语句 |
| ---------------------------------------------------- | -------------- |
| UPDATE bar SET age = 25 WHERE name = 'Harry';        | db.foo.bar.update( { name: "Harry" },{ $set: { age: 25 } } )    |
| UPDATE bar SET age = age + 2 WHERE name = 'Harry';   | db.foo.bar.update( { name: "Harry" },{ $inc: { age: 2 } } )     |

##Delete##

下表列出了在表上的删除记录：

| SQL 语句 | SequoiaDB 语句 |
| ---------------------------------- | ------------------------------------------- |
| DELETE FROM bar WHERE age = 20;    | db.foo.bar.remove( { age: 20 } ) |
| DELETE FROM bar;                   | db.foo.bar.remove() |
