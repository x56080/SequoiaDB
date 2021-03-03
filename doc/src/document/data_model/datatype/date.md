##概念##

SequoiaDB 中的日期使用 YYYY-MM-DD 的形式存取，在存储时将其转换为 8 字节的整数。

取值范围为：0000-01-01 至 9999-12-31。

##格式##

日期的表达形式如下：

```lang-json
{ "$date" : "<YYYY-MM-DD>" }
```

>  **Note:**
>
>  请参考 [SdbDate](reference/Sequoiadb_command/SpecialObjects/SdbDate.md)。

##示例##

```lang-json
{ "createTime" : { "$date" : "2012-05-12" } }
```
