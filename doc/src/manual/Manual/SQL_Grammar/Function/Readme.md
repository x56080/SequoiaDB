SequoiaDB 巨杉数据库可以通过函数对指定数据进行统计，支持的函数如下：

| 名称 | 描述 | 示例 |
|------|------|------|
| sum() | 对指定字段所有值的求和 | db.exec( "select sum(age) as 年龄总和 from sample.employee" ) |
| count() | 记录指定字段名的条数 |db.exec("select count(age) as 数量 from sample.employee") |
| avg() | 求指定字段名的平均值 | db.exec("select avg(age) as 平均年龄 from sample.employee") |
| max() | 指定字段在记录中的最大值 | db.exec("select max(a) as 最大值 from sample.employee") |
| min() | 指定字段在记录中的最小值 | db.exec("select min(a) as 最小值 from sample.employee") |
| first() | 指定字段范围内的第一条数据 | db.exec( "select first(a) as a, b from sample.employee group by b" ) |
| last() | 指定字段的最后一条数据 | db.exec( "select last(a) as a, b from sample.employee group by b" ) |
| push() | 将多个值合并为一个数组 | db.exec( "select a, push(b) as b from sample.employee group by a" ) |
| addtoset() | 将集合中多条记录中的相同字段的值合并到一个没有重复值的数组中 | db.exec("select a, addtoset(b) as b from sample.employee group by a") |
| buildobj() | 将记录中多个字段合并为一个对象 | db.exec("select a, buildobj(b, c) as d from sample.employee") |
| mergearrayset() | 将字段合并为一个不包含重复值的数组字段 | db.exec( "select a, mergearrayset(b) as b from sample.employee group by a" ) |