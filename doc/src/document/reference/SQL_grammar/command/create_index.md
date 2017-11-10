用于在集合中创建索引。在不读取整个集合的情况下，索引使数据库应用程序能够更快地查找数据。

##语法##
***create [unique] index \<index_name\> on \<cs_name\>.\<cl_name\> (field1_name [asc/desc],...)***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| unique| string | 表示创建唯一索引。唯一索引用于改善性能和保证数据完整性。唯一索引不允许集合中具有重复的值，除此之外，它与普通索引功能一样。 | 否 |
| index_name| string | 索引名称。 | 是 |
| cs_name | string | 集合空间名。 | 是 |
| cl_name | string | 集合名。 | 是 |
| field1_name | string | 创建索引所使用的字段名。可使用多个字段创建组合索引。 | 是 |
| asc/desc | string | asc表示创建索引所指定的字段的值将按升序排列；desc表示创建索引所指定的字段的值将按降序排列。 | 否 |

##返回值##
无。

## 示例##
   在集合 foo.bar 中创建若干索引。

   * 单字段索引。

   ```lang-javascript
   // 使用"age"字段创建一个名为"test_index1"的索引。
   > db.execUpdate("create index test_index1 on foo.bar (age)")
   Takes 0.2712s.

   // 如果希望索引中"age"的字以降序排列，可以在字段名后面添加保留字desc。
   > db.execUpdate("create index test_index2 on foo.bar (age desc)")
   Takes 0.3522s.
   ```

   * 组合索引。

   ```lang-javascript
   // 可以在括号中列出需要使用的字段，用逗号隔开。
   > db.execUpdate("create index test_index3 on foo.bar (age desc,name asc)")
   Takes 0.2752s.
   ```

   * 唯一索引。

   ```lang-javascript
   // 使用"age"字段创建一个名为"test_index4"的唯一索引。
   > db.execUpdate("create unique index test_index4 on foo.bar (age)")
   Takes 0.2972s.
   ```
