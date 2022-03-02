[^_^]:
    FlinkSQL 连接器-使用

本文档主要介绍如何通过 Flink 向 SequoiaDB 巨杉数据库写入和读取数据。

##数据写入##

###语法###

```lang-sql
insert:
   INSERT { INTO | OVERWRITE } [catalog_name.][db_name.]table_name VALUES values_row [, values_row ...]

insert_into_select:
   INSERT { INTO | OVERWRITE } [catalog_name.][db_name.]table_name
   select_statement    

select_statment:
    SELECT [ ALL | DISTINCT ] { * | projectItem [, projectItem]* }
    FROM table_expression
    [ WHERE boolean_expression ]
    [ GROUP BY groupItem1 [, groupItem]* ]
    [ HAVING boolean_expression ] 
```

###示例###

在映射表 employee 中插入如下数据：

```lang-sql
Flink SQL> INSERT INTO employee VALUES (1, "Jacky", 36);
Flink SQL> INSERT INTO employee VALUES (2, "Alice", 18);
```

##数据读取##

###语法###

```lang-sql
syntax:
{
    select_statement
  | query UNION [ ALL ] query
  | query EXCEPT query
  | query INTERSECT query
}
[ ORDER BY order_item [, order_item]* ]
[ LIMIT {count | ALL} ]
[ OFFSET start_pos {ROW | ROWS} ]
[ FETCH {FIRST | NEXT} | [count] {ROW | ROWS} only ]

order_item:
    expression [ ASC | DESC ]

select_statment:
    SELECT [ ALL | DISTINCT ] { * | projectItem [, projectItem]* }
    FROM table_expression
    [ WHERE boolean_expression ]
    [ GROUP BY groupItem1 [, groupItem]* ]
    [ HAVING boolean_expression ]

table_expression:
    table_name [, table_name]*
   | table_expression [ LEFT | RIGHT | FULL ] JOIN table_expression join_cond
   | sub_query

join_cond:
   ON boolean_expression
   | USING '(' column [, column ]* ')'

projectItem:
    expression [ [ AS ] columnAlias ]

groupItem:
    expression
   | '(' ')'
   | '(' expression [, expression ]* ')'
   | CUBE '(' expression [, expression ]* ')'
   | ROLLUP '(' expression [, expression ]* ')'
   | GROUPING SETS '(' groupItem [, groupItem ]* ')'
```

###示例###

读取表 employee 的所有数据

```lang-sql
Flink SQL> SELECT * FROM employee;
```



