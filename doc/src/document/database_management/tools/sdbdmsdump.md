sdbdmsdump（1.8 版本前名为 sdbinspt，1.8 版本后更名为 sdbdmsdump）是一个 SequoiaDB 数据库的数据文件检测工具。它可以检查数据库文件结构的正确性，并且给出结果报告。

##权限需求##

运行 sdbdmsdump 命令的用户必须对数据库的数据与索引文件拥有读权限。

##连接需求##

sdbdmsdump 不需要与数据库连接。

##选项##

| 参数        | 参数 | 描述                                              |
| ----------- | ---- | ------------------------------------------------- |
| --help      | -h   | 返回基本帮助和用法文本                            |
| --dbpath    | -d   | 指定数据库文件所在目录，默认为当前目录            |
| --output    | -o   | 指定输出文件，默认为屏幕输出                      |
| --verbose   | -v   | 是否进行 ASCII 文本输出（true 或 false），默认值为 true |
| --csname    | -c   | 指定集合空间名，如果未指定则为全部集合空间        |
| --clname    | -l   | 指定集合名，如果未指定则为全部集合                |
| --action    | -a   | 指定操作，为 inspect、dump 或 all 之一，<b>必须指定</b><br> - inspect：检测并报告任何数据损坏<br> - dump：将数据页格式化并输出<br> - all：检测数据页损坏，并格式化输出数据页 |
| --dumpdata  | -t   | 设定操作数据文件（true 或 false），默认值为 false |
| --dumpindex | -i   | 设定操作索引文件（true 或 false），默认值为 false |
| --pagestart | -s   | 指定起始数据页，默认为 -1                         |
| --numpage   | -n   | 指定需要检测或格式化的数据页数量，当指定 -s 参数为非负值时，该参数生效。默认值为 1 |
| --record    | -p   | 指定显示格式化输出数据或索引内容（true 或 false），默认值为 false |

##用法##

使用 sdbdmsdump 工具时，请务必保证数据库进程已经停止。

在下面的例子，sdbdmsdump 在当前目录下检测，并格式化输出所有集合空间与集合的数据与索引至 output.txt 文件。

```lang-javascript
$ sdbdmsdump -d . -o output.txt -v true -a all -t true -i true -p true
```
