sdbseadapter 是 SequoiaDB 与 Elasticsearch 连接的桥梁，以在 SequoiaDB 上支持全文检索能力的工具。

##选项##

| 参数              | 缩写 | 描述                                     |
| ----------------- | ---- | ---------------------------------------- |
| --help            | -h   | 帮助选项                                 |
| --version         |      | 版本信息                                 |
| --confpath        | -c   | 配置文件路径（不需指定文件名）           |
| --diaglevel       | -v   | 日志级别，默认值：3                      |
| --datanodehost    |      | 数据节点所在主机名                       |
| --datasvcname     |      | 数据节点服务端口号                       |
| --searchenginehost|      | 搜索服务器（Elasticsearch）所在主机名    |
| --searchengineport|      | 搜索服务器（Elasticsearch）服务端口号，默认值：9200 |
| --idxprefix       | -p   | 全文检索适配器在搜索服务器（Elasticsearch）上创建索引时使用的索引名前缀，可包含英文字母（大小写不敏感）、阿拉伯数字及下划线，且不能以下划线开头，最大长度16个字符，默认值为空。注意：同一个复制组对应的适配器必须使用相同的前缀，不同的 SequoiaDB 集群在共用相同的搜索服务器时，必须配置不同的前缀名。建议一个 SequoiaDB 集群中的所有的适配器使用相同的前缀 |
| --bulkbuffsize    |      | 批量操作缓存大小，范围 [1, 32]，单位：MB，默认值：10 |
| --optimeout       | -t   | 在搜索服务器（Elasticsearch）上操作的超时时间，范围 [3000, 3600000]，单位：ms，默认值：10000|
| --stringmaptype   | -s   | 字符串类型（包括字符串数组）的字段在搜索服务器（Elasticsearch）上映射的类型，范围 [1, 3]，默认值：1。值 1 表示 "text" 类型，字符串在搜索引擎上会被分析和拆词，适合于全文检索场景；值 2 表示 "keyword"，字符串不会被拆词，适合于字符串的精确查询、聚合运算等；值 3 表示同时映射成 "text" 和 "keyword" 类型，直接使用字段名时使用的是其 "text" 类型，而要使用其 "keyword" 类型，则需要使用 field_name.keyword 的格式。详细内容可参考 Elasticsearch 文档中的[字段类型介绍](https://www.elastic.co/guide/en/elasticsearch/reference/6.2/mapping-types.html)|
| --connlimit       | -l   | 全文检索适配器与搜索服务器之间的连接数上限，范围 [1, 65535]，默认值：50|
| --conntimeout     | -o   | 全文检索适配器与搜索服务器之间连接空闲时的超时时间，超时后连接将被释放，范围 [60-86400]，单位：秒，默认值：1800 |
| --scrollsize      |      | 全文检索适配器使用 scroll 方式（查询条件中不设置 from/size 参数）从搜索服务器（Elasticsearch）获取查询结果时，每批结果的记录数，范围 [50, 10000]，默认值：1000 |

适配器与数据节点一一对应，每个适配器需要使用一个单独的配置文件（或手工指定启动参数）。在安装路径下的 `conf/samples` 目录中有配置文件模板 `sdbseadapter.conf`，可将该文件拷贝到目标路径下并按实际组网进行配置。建议在数据节点的 `conf` 目录下创建单独的目录（如 `seadapter`），并在该目录中按节点服务端口号创建子目录，分别存放各自的配置文件。