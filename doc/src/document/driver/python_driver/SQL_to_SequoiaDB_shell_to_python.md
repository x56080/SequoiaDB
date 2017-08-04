SequoiaDB 的查询用 dict（bson）对象表示，下表以例子的形式显示了 SQL 语句，SequoiaDB shell 语句和 SequoiaDB Python 驱动程序语法之间的对照。

| SQL                                           | SequoiaDB shell                                     | Python Driver                                            |
| --------------------------------------------- | --------------------------------------------------- | -------------------------------------------------------- |
| insert into bar(a,b) values(1,-1)             | db.foo.bar.insert({a:1,b:-1})                       | cl = db.get_collection("foo.bar")<br>obj = { "a":1, "b":-1 }<br>cl.insert( obj ) |
| select a,b from bar                           | db.foo.bar.find(null,{a:"",b:""})                   | cl = db.get_collection("foo.bar")<br>selected = { "a":"","b":"" }<br>cr = cl.query(selector = selected ) |
| select * from bar                             | db.foo.bar.find()                                   | cl = db.get_collection("foo.bar")<br>cr = cl.query () |
| select * from bar where age=20                | db.foo.bar.find({age:20})                           | cl = db.get_collection("foo.bar")<br>cond ={"age":20}<br>cr = cl.query ( condition = cond ) |
| select * from bar where age=20 order by name  | db.foo.bar.find({age:20}).sort({name:1})            | cl = db.get_collection("foo.bar")<br>cond ={"age":20}<br>orderBy = {"name":1}<br>cr = cl.query (condition=cond , order_by=orderBy) |
| select * from bar where age > 20 and age < 30 | db.foo.bar.find({age:{$gt:20,$lt:30}})              | cl = db.get_collection("foo.bar")<br>cond = {"age":{"$gt":20,"$lt":30}}<br>cr = cl.query (condition = cond ) |
| create index testIndex on bar(name)           | db.foo.bar.createIndex("testIndex",{name:1},false)  | cl = db.get_collection("foo.bar")<br>obj = { "name":1 }<br>cl.create_index ( obj, "testIndex", False, False ) |
| select * from bar limit 20 offset 10          | db.foo.bar.find().limit(20).skip(10)                | cl = db.get_collection("foo.bar")<br>cr = cl.query (num_to_skip=10L, num_to_return=20L ) |
| select count(*) from bar where age > 20       | db.foo.bar.find({age:{$gt:20}}).count()             | cl = db.get_collection("foo.bar")<br>count = 0L<br>condition = { "age":{"$gt":20}}<br>count = cl.get_count (condition ) |
| update bar set a=2 where b=-1                 | db.foo.bar.update({$set:{a:2}},{b:-1})              | cl = db.get_collection("foo.bar")<br>condition = { "b":1 }<br>rule = { "$set":{"a":2} }<br>cl.update ( rule, condition=condition ) |
| delete from bar where a=1                     | db.foo.bar.remove({a:1})                            | cl = db.get_collection("foo.bar")<br>condition = {"a":1}<br>cl.delete ( condition=condition ) |
