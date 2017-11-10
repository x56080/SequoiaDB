使用hint显示地控制执行计划。


##语法##
___/*+hint1 hint2 ...*/___

当我们希望控制某个 select 语句时，只需要在这个 select 语句结尾处增加 hint 即可。属于同一个 select 语句的 hint 使用空格分隔。

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| hint1/hint2 | function | use_hash()或use_index()函数，表示指定使用的索引。 | 是 |

##返回值##
无。

##示例##
   * 数据库中集合 foo.bar1, foo.bar2, foo.bar3 的情况如下。

   ```
   // foo.bar1包含索引 "idx_bar1_a"，该索引以 "a" 字段升序排序，其记录如下：
   { "a": 0 }
   { "a": 1 }
   { "a": 2 }
   { "a": 3 }
   { "a": 4 }
   
   // foo.bar2包含索引 "idx_bar2_b"，该索引以 "b" 字段升序排序，其记录如下：
   { "a": 1, "b": 1 }
   { "a": 2, "b": 1 }
   { "a": 3, "b": 2 }
   { "a": 4, "b": 2 }
   { "a": 5, "b": 2 }

   // foo.bar1包含索引 "idx_bar3_c"，该索引以 "c" 字段升序排序，其记录如下：
   // foo.bar3
   { "c": 0 }
   { "c": 1 }
   { "c": 2 }
   { "c": 3 }
   { "c": 4 }
   ```

###use_hash()###

   * 指定关联方式为哈希关联。

   ```lang-javascript
   > db.exec("select t1.a, t2.b from foo.bar1 as t1 inner join foo.bar2 as t2 on t1.a = t2.b /*+use_hash()*/")
   { "a": 1, "b": 1 }
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   Return 5 row(s).
   Takes 0.16533s.
   ```

###use_index()###

   * 指定集合的扫描方式

   * 使用集合 foo.bar1 中索引 "idx_bar1_a" 进行扫描。

   ```lang-javascript
   > db.exec("select * from foo.bar1 where a = 1 /*+use_index(idx_bar1_a)*/")
   { "_id": { "$oid": "582ae8ea790ce72860000023" }, "a": 1 }
   Return 1 row(s).
   Takes 0.4843s.
   ```

   * 在关联中指定集合 foo.bar1 使用索引 "idx_bar1_a" 进行扫描。

   ```lang-javascript
   > db.exec("select t1.a, t2.b from foo.bar1 as t1 inner join foo.bar2 as t2 on t1.a = t2.b /*+use_index(t1, idx_bar1_a)*/")
   { "a": 1, "b": 1 }
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   Return 5 row(s).
   Takes 0.13962s.
   ```

   * 在一个 select 语句中使用多个 hint。

   ```lang-javascript
   > db.exec("select t1.a, t2.b from foo.bar1 as t1 inner join foo.bar2 as t2 on t1.a = t2.b /*+use_index(t1, idx_bar1_a) use_index(t2, idx_bar2_b) use_hash()*/")
   { "a": 1, "b": 1 }
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   Return 5 row(s).
   Takes 0.18535s.
   ```

   * 指定集合不使用索引。

   ```lang-javascript
   > db.exec("select * from foo.bar1 where a = 1 /*+use_index(NULL)*/")
   { "_id": { "$oid": "582ae8ea790ce72860000023" }, "a": 1 }
   Return 1 row(s).
   Takes 0.4843s.
   ```
