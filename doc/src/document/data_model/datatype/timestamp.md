##概念##

SequoiaDB 中的时间戳使用 YYYY-MM-DD-HH.mm.ss.ffffff 的形式存取，在存储时将其转换为 8 字节的整数。

取值范围为：1902-01-01 00:00:00.000000 至 2037-12-31 23:59:59.999999。

##格式##

时间戳的表达形式如下：

```
{ "$timestamp" : "<YYYY-MM-DD-HH.mm.ss.ffffff>" }
```

>  **Note:**
>
>  请参考 [Timestamp](reference/Sequoiadb_command/SpecialObjects/Timestamp.md)

##示例##

```
{ "createTime" : { "$timestamp" : "2012-05-12-13.15.21.241523" } }
```
