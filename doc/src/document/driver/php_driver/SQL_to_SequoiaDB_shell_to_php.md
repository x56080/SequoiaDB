SequoiaDB 的查询用json（bson）对象表示，下表以例子的形式显示了SQL语句，SequoiaDB shell语句和SequoiaDB PHP驱动程序语法之间的对照关系。

| SQL                                           | SequoiaDB shell                                     | PHP Driver                                       |
| --------------------------------------------- | --------------------------------------------------- | ------------------------------------------------ |
| insert into bar(a,b) values(1,-1)             | db.foo.bar.insert({a:1,b:-1})                       | $bar->insert("{a:1,b:-1}")                       |
| select a,b from bar                           | db.foo.bar.find(null,{a:"",b:""})                   | $bar->find(NULL, '{a:"",b:""}')                  |
| select * from bar                             | db.foo.bar.find()                                   | $bar->find()                                     |
| select * from bar where age=20                | db.foo.bar.find({age:20})                           | $bar->find("{age:20}")                           |
| select * from bar where age=20 order by name  | db.foo.bar.find({age:20}).sort({name:1})            | $bar->find("{'age':20}", NULL, "{'name':1}")     |
| select * from bar where age > 20 and age < 30 | db.foo.bar.find({age:{$gt:20,$lt:30}})              | $bar->find('{age:{$gt:20,$lt:30}}')              |
| create index testIndex on bar(name)           | db.foo.bar.createIndex("testIndex",{name:1},false)  | $bar->createIndex("{name:1}", "testIndex", false)|
| select * from bar limit 20 offset 10          | db.foo.bar.find().limit(20).skip(10)                | $bar->find(NULL, NULL, NULL, NULL, 10, 20)       |
| select count(*) from bar where age > 20       | db.foo.bar.find({age:{$gt:20}}).count()             | $bar->count('{age:{$gt:20}}')                    |
| update bar set a=a+2 where b=-1               | db.foo.bar.update({$inc:{a:2}},{b:-1})              | $bar->update('{$inc:{a:2}}', "{b:-1}")           |
| delete from bar where a=1                     | db.foo.bar.remove({a:1})                            | $bar->remove("{a:1}")                            |
