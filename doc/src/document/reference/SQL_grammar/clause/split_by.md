按照某个数组字段将记录拆分。

## 语法##
***split by \<field_name\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field_name | string | 字段名。 | 是 |

##返回值##
无。

##示例##
 
   * 集合 foo.bar 中原始记录。

   ```
   { a: 1, b: 2, c: [3, 4, 5] }
   { a: 2, b: 3, c: [6, 7] }
   ```

   * 对集合 foo.bar 按字段c拆分。

   ```lang-javascript
   > db.exec( "select a,b,c from foo.bar split by c" )
   { "a": 1, "b": 2, "c": 3 }
   { "a": 1, "b": 2, "c": 4 }
   { "a": 1, "b": 2, "c": 5 }
   { "a": 2, "b": 3, "c": 6 }
   { "a": 2, "b": 3, "c": 7 }
   Return 5 row(s).
   Takes 0.5281s.
   ```
