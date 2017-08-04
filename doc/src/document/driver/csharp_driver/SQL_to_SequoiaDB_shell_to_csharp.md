SequoiaDB 的查询用 json（bson）对象表示，下表以例子的形式显示了 SQL 语句，SequoiaDB shell 语句和 SequoiaDB CSharp 驱动程序语法之间的对照。

| SQL                                                        | SequoiaDB shell                                            | CSharp Driver                                                          |
| ---------------------------------------------------------- | ---------------------------------------------------------- | ------------------------------------------------------------------ |
| insert into bar( a, b ) values( 1, -1 )                    | db.foo.bar.insert( { a: 1, b: -1 } )                       | bar.insert( "{ 'a': 1, 'b': -1 }" )                                |
| select a,b from bar                                        | db.foo.bar.find( null, { a: "", b: "" } )                  | bar.query( "", "{ 'a': '', 'b': '' }", "", "" )                    |
| select * from bar                                          | db.foo.bar.find()                                          | bar.query()                                                        |
| select * from bar where age=20                             | db.foo.bar.find( { age: 20 } )                             | bar.query( "{ 'age': 20 }", "", "", "" )                           |
| select * from bar where age=20 order by name               | db.foo.bar.find( { age: 20 } ).sort( { name: 1 } )         | bar.query( "{ 'age': 20 }", "", "{ 'name': 1 }", "" )              |
| select * from bar where age > 20 and age < 30              | db.foo.bar.find( { age: { $gt: 20, $lt: 30 } } )           | bar.query( "{ 'age': { '$gt': 20, '$lt': 30 } }" , "", "", "" )    |
| create index testIndex on bar( name )                      | db.foo.bar.createIndex( "testIndex", { name: 1 }, false )  | bar.createIndex( "testIndex", "{ 'name': 1 }", false, false )      |
| select * from bar limit 20 offset 10                       | db.foo.bar.find().limit( 20 ).skip( 10 )                   | bar.query( "", "", "", "", 10, 20 )                                |
| select count(*) from bar where age > 20                    | db.foo.bar.find( { age: { $gt: 20 } } ).count()            | bar.getCount( "{ 'age': { '$gt': 20 } }" )                         |
| update bar set a=a+2 where b=-1                            | db.foo.bar.update( { $inc: { a: 2 } },{ b: -1 } )          | bar.update( "{ 'b': -1 }", "{ '$inc': { 'a': 2 } }", "" )          |
| delete from bar where a=1                                  | db.foo.bar.remove( { a: 1 } )                              | bar.delete( "{ 'a': 1 }" )                                         |
