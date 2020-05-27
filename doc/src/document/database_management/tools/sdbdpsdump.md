sdbdpsdump 是一个 SequoiaDB 数据库的同步日志文件 dump 工具。它可以解析 [replica log 同步日志](database_management/database_configuration/special_configuration_modify/log_synchronization.md)文件的内容，并且给出结果报告。

##权限需求##

运行 sdbdpsdump 命令的用户必须对数据库的 replica log 同步日志文件拥有读权限。

##连接需求##

sdbdpsdump 不需要与数据库连接。

##选项##

| 参数        | 参数 | 描述 |
| ----------- | ---- | ---- |
| --help      | -h   | 返回基本帮助和用法文本 |
| --version   | -v   | 返回工具的编译版本 |
| --meta      | -m   | 指定在解析日志文件时，只解析日志文件的元数据信息，只能单独使用 |
| --type      | -t   | 指定只解析日志文件中指定类型的日志：<br> 1：数据插入<br> 2：数据更新<br> 3：数据删除<br> 4：创建集合空间<br> 5：删除集合空间<br> 6：创建集合<br> 7：删除集合<br> 8：创建索引<br> 9：删除索引<br> 10：集合重命名<br> 11：集合truncate<br> 12：事务提交<br> 13：事务回滚<br> 14：清空Catalog缓存<br> 15：写入LOB数据<br> 16：删除LOB数据<br> 17：修改LOB数据<br> 18：LOB数据truncate<br> 21：修改集合/集合空间/域的属性<br> 22：添加集合/集合空间的UniqueID |
| --name      | -n   | 指定集合空间或者集合名，只解析和指定名相关的集合或者集合空间的日志 |
| --lsn       | -l   | 指定 lsn 值，只解析 lsn 的日志，可与 -a 和 -b 联用 |
| --last      | -e   | 指定解析日志文件最后的N条日志，N 为指定的数值（需要 -s 指定到文件） |
| --source    | -s   | 指定日志文件的目录或路径，默认为当前目录 |
| --output    | -o   | 指定输出文件，默认为屏幕输出 |
| --ahead     | -a   | 指定解析指定 lsn 之前的 N 条日志，N 为指定数值，必须与 -l 或 --lsn 联用，默认值为 20 |
| --back      | -b   | 指定解析指定 lsn 之后的 N 条日志，N为指定数值，必须与 -l 或 --lsn 联用，默认值为 20 |

>   **Note:**  
>   sdbdpsdump不管有没指定--meta选项，都会打印日志文件的元数据信息。

##用法##


在下面的例子，sdbdpsdump 解析指定目录中的 replica log 文件，并只解析类型为 1（数据插入）的日志，输出到当前目录 out.log：

```lang-bash
$ sdbdpsdump -s /data/database/40000/replicalog/sequoiadbLog.0 -o out.log -t 1
```
