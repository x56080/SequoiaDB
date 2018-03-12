用于过滤记录。

##语法##

***where \<comparison_expression\>***

##参数##

| 参数名          | 参数类型 | 描述       | 是否必填 |
|-----------------|----------|------------|----------|
| comparison_expression | expression | [比较表达式](reference/SQL_grammar/expression.md#比较表达式) | 是 |

##示例##

   * 集合 foo.bar 中存在如下记录。

   ```
   { "name": "Lucy", "age": 11 }
   { "name": "Sam", "age": 8 }
   { "name": "Tom", "age": 7 }
   { "name": "James", "age": 12 }
   ```

   * 查询匹配集合中 age 大于 10 的记录。

   ```lang-javascript
   > db.exec("select * from foo.bar where age > 10")
   { "name": "Lucy", "age": 11 }
   { "name": "James", "age": 12 }
   Return 2 row(s).
   Takes 0.6149s.
   ```
