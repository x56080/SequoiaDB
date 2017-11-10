用于修改集合中的记录。

##语法##
***update \<cs_name\>.\<cl_name\> set (\<field1_name\>=\<value1\>,...) [where \<condition\>]***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| cs_name | string | 集合空间名。 | 是 |
| cl_name | string | 集合空间名。 | 是 |
| field1_name | string | 字段名。 | 是 |
| value1 | string | 字段值。 | 是 |
| condition | expression | 条件，只对符合条件的记录更新。 | 是 |

##返回值##
无。

##示例##
 
   * 集合 foo.bar 中原始记录。

   ```
   { age: 1 }
   { age: 2 }
   ```

   * 修改集合中全部的记录，将记录中的 age 字段值修改为20。

   ```lang-javascript
   > db.execUpdate( "update foo.bar set age=20" )
   Takes 0.1511s.
   ```

   * 修改符合条件的记录，只对符合条件 age \< 25 的记录做修改操作。

   ```lang-javascript
   > db.execUpdate( "update foo.bar set age=30 where age < 25" )
   Takes 0.3023s.
   ```
