sdbinspect 是 SequoiaDB 数据一致性检测工具，可以检查节点间数据是否完全一致，并且给出结果报告。

##权限需求##

无

##连接需求##

sdbinspect 需要与数据库协调节点（Coord 节点）连接。

##选项##

| 参数              | 缩写 | 描述 |
| ----------------- | ---- | ---- |
| --help            | -h   | 返回基本帮助和用法文本 |
| --version         | -v   | 返回当前工具所附属的数据库的版本 |
| --action          | -a   | 指定检查数据或对已经存在的中间文件生成 report，inspect 和 report 可选，默认是 inspect |
| --coord           | -d   | 指定协调节点（Coord 节点）的 hostname 和服务端口，格式为 hostname:servicename，默认值是 `localhost:11810` |
| --loop            | -t   | 指定迭代检查的次数，默认值为 5 |
| --group           | -g   | 指定要检查的组（group）的名字，若不指定，则检查所有的组（group） |
| --collectionspace | -c   | 指定检查的集合空间名字，不指定则检查所有集合空间 |
| --collection      | -l   | 指定检查的集合名字，不指定则检查所有集合，指定集合时，必须制定集合空间 |
| --file            | -f   | 指定从已存在的（上一次检查的）结果文件开始检查，当指定此选择时，其它选项（除  -o 外）均失效，生效的为文件中保存的 command 选项 |
| --output          | -o   | 指定输出的文件名，默认是 `inspect.bin`，报告文件为 `inspect.bin.report` |
| --view            | -w   | 指定生成 report 文件的内容按组（group）查看和按集合（collection）查看，默认为 group |
| --auth            | -u   | 指定数据库鉴权需要的用户名和密码，格式：username:password，默认值为 "":""。只提供用户名而不提供密码时，工具会通过交互式界面提示用户输入密码，格式: username |
| --cipher          |      | 是否使用密文模式输入密码，默认为 false，不使用密文模式输入密码。关于密文模式的介绍，详细可参考[密码管理](database_management/security/system_security.md) |
| --token           |      | 指定加密令牌 |
| --cipherfile      |      | 指定密文文件路径，默认值为 `~/sequoiadb/passwd` |

##用法##

* 在下面的例子，sdbinspect 检查协调节点 `hostname1:11810` 下的全部集群（5次），并将中间文件结果输出到 `item.bin` 中，同时会解析 `item.bin` 文件，把文本结果按（默认的）group 划分，输出到 `item.bin.report` 文件中

   ```lang-bash
   $ sdbinspect -d hostname1:11810 -o item.bin
   ```

* 在下面的例子，sdbinspect 检查协调节点 `hostname1:11810` 下的全部集群中的集合空间 sports（3次），并将中间文件结果输出到 `item.bin` 中，同时会解析 `item.bin` 文件，把文本结果按 collection 划分，输出到 `item.bin.report` 文件中

   ```lang-bash
   $ sdbinspect -d hostname1:11810 -o item.bin -c sports -w collection -t 3
   ```

* 在下面的例子，sdbinspect 检查协调节点 `hostname1:11810` 下的 group1 集群中的名为 sports 的集合空间，名为 item 的集合（5次），并将中间文件结果输出到 `inspect.bin` 中，同时会解析 `inspect.bin` 文件，把文本结果按（默认的）group 划分，输出到 `inspect.bin.report` 文件中

   ```lang-bash
   $ sdbinspect -d hostname1:11810 -g group1 -c sports -l item
   ```
