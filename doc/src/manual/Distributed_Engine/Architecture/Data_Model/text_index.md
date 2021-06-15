[^_^]:
    全文索引
    作者：林苏强
    时间：20190716
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190819


全文索引用于在大量文本中进行快速的检索。在使用普通索引时，搜索特定的关键字需要使用[正则表达式][regex]。当文本是整本书或是整篇文章时，正则表达式的效率较低。而全文索引会创建一个词库，统计每个词条出现的频率和位置。在搜索某词时，就可以快速定位到该词出现的位置，提升检索效率。

SequoiaDB 全文检索能够实现近实时的搜索能力，即一个新的文档从被索引到可被搜索会有一定的延迟。延迟取决于索引的速度。主要分两种情况：
- 在空集合或者只有很少量数据的集合上创建全文索引。在写入压力不是太大的情况下，通常在若干秒（典型值如 1~5 秒）内，新增的数据即可被搜索到。
- 创建索引时，集合中已存在大量的数据。此时要全量索引集合中的所有文档，耗时从分钟级到若干小时不等，取决于数据规模、搜索服务器性能等因素。如果在全量索引完成之前进行查询，只能查到部分结果。


基本原理
----

SequoiaDB 使用 Elasticsearch 作为全文检索引擎实现全文索引。全文索引与普通索引的最大区别在于，索引数据不是存在于数据节点的索引文件中， 而是存储在 Elasticsearch 中。在使用该索引进行查询的时候，会在 Elasticsearch 中进行搜索，数据节点根据其返回的结果，再到本地查找数据。实现时涉及以下三个角色：

- SequoiaDB 数据节点：存储数据
- Elasticsearch 集群：用于存储全文索引数据，以及在索引中进行搜索
- 适配器 sdbseadapter：作为 SequoiaDB 数据节点与 Elasticsearch 交互的桥梁，进行数据转换与传输等

例如，有 SequoiaDB 3 组 1 节点的集群和 Elasticsearch 集群。某集合数据均匀切分到所有数据组上。在该集合上使用全文索引进行检索，流程如图。

![avatar][full_text_search_flow]

协调节点先将请求分发至所有数据组，由数据节点转发搜索请求至 Elasticsearch 集群。Elasticsearch 在索引中搜索到结果后，再由数据节点将真实数据返回至协调节点，协调节点将数据进行汇总后，返回至客户端。


环境部署
----

SequoiaDB 通过与 Elasticsearch 配合提供全文检索能力。使用全文检索时必须完成 Elasticsearch 集群的部署，并配置好 SequoiaDB 的[搜索引擎适配器][sdbseadapter]。配置步骤可参考[全文检索环境部署][fulltext_deployment]。

搜索引擎适配器
----
sdbseadapter 是 SequoiaDB 与 Elasticsearch 连接的桥梁，是实现 SequoiaDB 支持全文检索能力的工具。

### 选项

| 参数名            | 缩写 | 描述                                     |
| ----------------- | ---- | ---------------------------------------- |
| --help            | -h   | 帮助选项                                 |
| --version         |      | 版本信息                                 |
| --confpath        | -c   | 配置文件路径（不需指定文件名）           |
| --diaglevel       | -v   | 日志级别，默认值为 3                      |
| --datanodehost    |      | 数据节点所在主机名                       |
| --datasvcname     |      | 数据节点服务端口号                       |
| --searchenginehost|      | 搜索服务器（Elasticsearch）所在主机名    |
| --searchengineport|      | 搜索服务器（Elasticsearch）服务端口号，默认值为 9200 |
| --idxprefix       | -p   | 全文检索适配器在搜索服务器（Elasticsearch）上创建索引时使用的索引名前缀，可包含英文字母（大小写不敏感）、阿拉伯数字及下划线，且不能以下划线开头，最大长度 16 个字符，默认值为空 <br> 注意：同一个复制组对应的适配器必须使用相同的前缀，不同的 SequoiaDB 集群在共用相同的搜索服务器时，必须配置不同的前缀名。建议一个 SequoiaDB 集群中的所有的适配器使用相同的前缀 |
| --bulkbuffsize    |      | 批量操作缓存大小，取值范围为[1,32]，单位为 MB，默认值为 10 |
| --optimeout       | -t   | 在搜索服务器（Elasticsearch）上操作的超时时间，取值范围为[3000,3600000]，单位为 ms，默认值为 10000|
| --stringmaptype   | -s   | 字符串类型（包括字符串数组）的字段在搜索服务器（Elasticsearch）上映射的类型，取值范围为[1,3]，默认值为 1 <br> 1：表示"text"类型，字符串在搜索引擎上会被分析和拆词，适合于全文检索场景 <br>2：表示"keyword"，字符串不会被拆词，适合于字符串的精确查询、聚合运算等 <br> 3：表示同时映射成"text"和"keyword"类型，直接使用字段名时使用的是其"text"类型，而要使用其"keyword"类型，则需要使用 field_name.keyword 的格式，详细内容可参考 Elasticsearch 文档中的[字段类型介绍](https://www.elastic.co/guide/en/elasticsearch/reference/6.2/mapping-types.html)|
| --connlimit       | -l   | 全文检索适配器与搜索服务器之间的连接数上限，取值范围为[1,65535]，默认值为 50|
| --conntimeout     | -o   | 全文检索适配器与搜索服务器之间连接空闲时的超时时间，超时后连接将被释放，取值范围为[60,86400]，单位为秒，默认值为 1800 |
| --scrollsize      |      | 全文检索适配器使用 scroll 方式（查询条件中不设置 from/size 参数）从搜索服务器（Elasticsearch）获取查询结果时，每批结果的记录数，取值范围为[50,10000]，默认值为 1000 |

适配器与数据节点一一对应，每个适配器需要使用一个单独的配置文件（或手工指定启动参数）。在安装路径下的 `conf/samples` 目录中有配置文件模板 `sdbseadapter.conf`，可将该文件拷贝到目标路径下并按实际组网进行配置。建议在数据节点的 `conf` 目录下创建单独的目录（如 `seadapter`），并在该目录中按节点服务端口号创建子目录，分别存放各自的配置文件。


全文检索环境部署
----

全文检索功能需要在 SequoiaDB 集群环境下使用，单机模式暂不支持。要使用全文检索功能，需要完成 Elasticsearch 集群、SequoiaDB 集群及搜索引擎适配器部署。

由于在 Elasticsearch 中创建的索引的名字，是由集合的 Unique ID、原始索引名等元素组合而成，不同的 SequoiaDB 集群间这些值可能相同，因此建议每个 SequoiaDB 集群使用独立的 Elasticsearch 集群，混用可能会造成数据错误。

### 软件安装

1. SequoiaDB 及搜索引擎适配器安装

   SequoiaDB 的搜索引擎适配器已包含在软件发布包中，按照 SequoiaDB 的安装步骤正常完成安装即可。适配器可执行程序为安装目录下的 `bin/sdbseadapter`。

2. Elasticsearch 安装 

   用户自行到 [Elasticsearch 官网](http://www.elastic.co)下载 Elasticsearch 安装包，并按照实际业务需要，参考 Elasticsearch 相关文档完成软件安装及集群部署。当前 SequoiaDB 适配的 Elasticsearch 版本为 6.8.5。

### 配置全文检索运行环境

- SequoiaDB 及 Elasticsearch 部署

   用户可以参考 SequoiaDB 及 Elasticsearch 的相关指导，完成 SequoiaDB 及 Elasticsearch 集群的部署，并确保其正常运行。

- 搜索引擎适配器部署

   1. 适配器节点配置文件准备

     每一个数据节点（包括主节点和备节点）需要启动一个对应的适配器节点，二者需要运行在同一台主机上。适配器启动的时候必需指定配置文件路径，且一个配置文件只能启动一个适配器实例。尝试使用同一个配置文件启动多个适配器实例将会失败。

     当需要使用全文检索功能时，在 SequoiaDB 安装目录的 `conf` 目录下，创建 `seadapter` 目录，并在该目录下，按适配器对应的数据节点的服务端口号，分别创建下层子目录并存放一份配置文件。配置文件模板可从 `conf/samples/sdbseadapter.conf` 拷贝，文件名应保持一致，然后依次对配置文件内容进行修改。如下以 SequoiaDB 安装路径为 `/opt/sequoiadb`，数据节点服务端口号分别为 11830、11840、11850 为例进行说明：

     ```lang-bash
     $ cd /opt/sequoiadb/conf
     $ mkdir seadapter
     $ cd seadapter
     $ mkdir 11830 11840 11850
     $ cp ../samples/sdbseadapter.conf 11830
     $ cp ../samples/sdbseadapter.conf 11840
     $ cp ../samples/sdbseadapter.conf 11850
     ```

     分别修改上述配置文件，填写数据节点及 Elasticsearch 的地址信息。如 11830 下配置文件内容如下（IP 及服务端口号按实际填写）：

     ```lang-ini
     datanodehost=192.168.1.123
     datasvcname=11830
     searchenginehost=192.168.1.124
     searchengineport=9200
     diaglevel=3
     optimeout=30000
     bulkbuffsize=10
     ```

   2. 适配器节点启动

     目前适配器进程通过手工方式启动，通过 -c 指定配置文件路径（不需要带配置文件名）

     ```lang-bash
     $ nohup sdbseadapter -c /opt/sequoiadb/conf/seadapter/11830 &
     $ nohup sdbseadapter -c /opt/sequoiadb/conf/seadapter/11840 &
     $ nohup sdbseadapter -c /opt/sequoiadb/conf/seadapter/11850 &
     ```

     使用 ps 命令查看是否所有适配器进程均已启动成功

     ```lang-bash
     $ ps -ef | grep sdbseadapter
     ```

     参考结果如下，括号内为其监听搜索请求的端口号：

     ```lang-text
     sdbseadapter(11837) A
     sdbseadapter(11847) A
     sdbseadapter(11857) A
     ```

使用
----

### 创建全文索引

全文索引可以指定一个或多个字段，普通[索引][index]的其它选项（如 Unique, NotNull...）均对全文索引无效，无需指定。例如，在 sample.employee 集合上为 name 及 address 字段创建复合全文索引，使用语句如下：
```lang-bash
db.sample.employee.createIndex( 'fulltext_idx', { 'name': 'text', 'address': 'text' } )
```
> **Note:**
>
> - 创建全文索引可参考 [createIndex()][create_index]
> - 只有字符串类型的字段会被索引，非字符串字段会被忽略。
> - 使用全文索引时，不要编辑文档自动生成的 _id 字段及其唯一索引 $id。如果 _id 数据类型被更改，或值不唯一等，文档都有可能无法被索引，导致全文检索查询结果不全。
> - 一个集合最多创建一个全文索引。数据库中最多创建 64 个全文索引。
> - 全文索引与其它索引不能混合使用，错误示例：`{"name": "text", "id": 1 }`。

### 使用全文索引检索

SequoiaDB 通过在查询中指定 Elasticsearch 的搜索条件来进行全文检索。
```lang-bash
find( { "": { "$Text": <search command> } } )
```
其中 \<search command\> 即 Elasticsearch 的搜索条件，需要使用 Elasticsearch 的 [DSL][dsl]（Domain Specific Language）语法。详情可参考 [Elasticsearch DSL][dsl] 官方文档。

**示例**

1. 创建集合 sample.employee

   ```lang-javascript
   > var cl = db.createCS('sample').createCL('employee')
   ```

2. 创建全文索引

   ```lang-javascript
   > cl.createIndex('idx_1', {first_name:"text", "last_name":"text", "age":"text", "about":"text", "interests": "text"})
   ```

3. 将数据插入 sample.employee 中

   ```lang-javascript
   > cl.insert({"first_name" : "John","last_name" : "Smith","age" : 25,"about" : "I love to go rock climbing","interests": [ "sports", "music" ]})
   > cl.insert({"first_name" : "Jane","last_name" : "Smith","age" : 32,"about" : "I like to collect rock albums","interests": [ "music" ]})
   > cl.insert({"first_name" : "Douglas","last_name" : "Fir","age" : 35,"about": "I like to build cabinets","interests": [ "forestry" ]})
   ```

4. 使用全文索引查找集合 sample.employee 中 name 包含"Smith"的所有记录

   ```lang-javascript
   > cl.find({"":{"$Text":{"query":{"match":{"about" : "rock climbing"}}}}}).hint({"":"idx_1"})
   {
     "_id": {
       "$oid": "5a8f8d9c89000a0906000000"
     },
     "first_name": "John",
     "last_name": "Smith",
     "age": 25,
     "about": "I love to go rock climbing",
     "interests": [
       "sports",
       "music"
     ]
   }
   {
     "_id": {
       "$oid": "5a8f8d9f89000a0906000001"
     },
     "first_name": "Jane",
     "last_name": "Smith",
     "age": 32,
     "about": "I like to collect rock albums",
     "interests": [
       "music"
     ]
   }
   Return 2 row(s).
   ```

### 删除全文索引

用户可使用 [dropIndex()][drop_index] 删除指定名称的全文索引。

```lang-bash
db.sample.employee.dropIndex( 'fulltext_idx' )
```



[^_^]:
     本文使用的所有链接和引用
[regex]:manual/Manual/Operator/Match_Operator/regex.md
[sdbseadapter]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md#搜索引擎适配器
[fulltext_deployment]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md#全文检索环境部署
[create_index]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[dsl]:https://elasticsearch-dsl.readthedocs.io/en/6.2.1/
[drop_index]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md
[full_text_search_flow]:images/Distributed_Engine/Architecture/Data_Model/full_text_search_flow.png
