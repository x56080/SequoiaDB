[^_^]:
    全文索引

SequoiaDB 巨杉数据库支持创建全文索引，用于在大量文本中进行快速检索。与其他索引相比，全文索引通过建立词库，统计每个词条出现的频率和位置，可以快速定位关键词出现的位置，以提升检索效率。

一个新的文档从插入集合到可被搜索会有一定的延迟，延迟长短取决于构建全文索引的速度。索引的构建主要分为以下两种情况：

- 在空集合或者只有很少量数据的集合上创建全文索引。在写入压力不大的情况下，通常在若干秒（典型值如 1~5 秒）内，新增的数据即可被搜索到。
- 创建全文索引时，集合中已存在大量的数据。此时要在集合的所有文档中构建索引，耗时从分钟级到若干小时不等，取决于数据规模、搜索服务器性能等因素。如果在索引构建完成之前进行查询，只能查到部分结果。

##基本原理##

SequoiaDB 巨杉数据库使用 Elasticsearch 作为全文检索引擎实现全文索引。全文索引与普通索引的最大区别在于，索引数据不是存在于数据节点的索引文件中， 而是存储在 Elasticsearch 中。全文索引的实现涉及以下三个角色：

- SequoiaDB 数据节点：用于管理数据
- Elasticsearch：用于管理全文索引数据
- 适配器 sdbseadapter：用于 SequoiaDB 数据节点与 Elasticsearch 间的数据转换、传输等

在集合上使用全文索引进行检索时，协调节点先将请求分发至集合相关的所有数据组，由数据节点转发搜索请求至 Elasticsearch 集群。Elasticsearch 搜索到相关索引信息后，数据节点根据索引信息查找数据，并将真实数据返回至协调节点，协调节点将数据汇总后返回至客户端。其流程图如下：

![avatar][full_text_search_flow]

##搜索引擎适配器##

sdbseadapter 是数据节点与 Elasticsearch 交互的桥梁。SequoiaDB 通过该工具实现全文索引功能，具体参数如下：

| 参数名            | 缩写 | 描述                                     |
| ----------------- | ---- | ---------------------------------------- |
| --help            | -h   | 获取帮助选项                                 |
| --version         |      | 获取版本信息                                 |
| --svcname         |      | sdbseadapter 节点端口号                  |
| --confpath        | -c   | 配置文件路径（不需指定文件名）           |
| --diaglevel       | -v   | 日志级别，默认值为 3                     |
| --datanodehost    |      | 数据节点所在主机名                       |
| --datasvcname     |      | 数据节点服务端口号                       |
| --searchenginehost|      | 搜索服务器（Elasticsearch）所在主机名    |
| --searchengineport|      | 搜索服务器（Elasticsearch）服务端口号，默认值为 9200 |
| --idxprefix       | -p   | 全文检索适配器在搜索服务器（Elasticsearch）上创建索引时使用的索引名前缀，默认值为空<br>前缀名最大长度为 16 个字符，可包含英文字母（大小写不敏感）、阿拉伯数字及下划线，但不能以下划线开头 <br>同一个复制组对应的适配器必须使用相同的前缀，不同的 SequoiaDB 集群在共用相同的搜索服务器时，必须配置不同的前缀名；建议一个 SequoiaDB 集群包含的所有适配器使用相同的前缀 |
| --bulkbuffsize    |      | 批量操作缓存大小，默认值为 10，单位为 MB，取值范围为[1, 32] |
| --optimeout       | -t   | 在搜索服务器（Elasticsearch）上操作的超时时间，默认值为 10000，单位为 ms，取值范围为[3000, 3600000]|
| --connlimit       | -l   | 全文检索适配器与搜索服务器之间的连接数上限，默认值为 50，取值范围为[1, 65535]|
| --conntimeout     | -o   | 全文检索适配器与搜索服务器之间连接空闲时的超时时间，超时后连接将被释放，默认值为 1800，单位为秒，取值范围为[60, 86400] |
| --scrollsize      |      | 全文检索适配器使用 scroll 方式（查询条件中不设置 from/size 参数）从搜索服务器（Elasticsearch）获取查询结果时，每批结果的记录数，默认值为 1000，取值范围为[50, 10000] |
| --logpath         |      | 适配器日志的存放路径，默认为 `<INSTALL_DIR>/conf/log/sdbseadapterlog/<port>` |

##索引字段映射##

在 SequoiaDB v3.6.1 及以上版本中，全文索引支持检索字符串类型以外的数据。关于字段类型，详细可参考 Elasticsearch 文档中的[字段类型介绍][mapping-types]。

###字段映射###

用户在创建全文索引时，可通过参数 Mappings 指定索引字段在 Elasticsearch 的映射关系，格式为 `Mappings: {"Fields": {<fieldName>: {...}, ...}} `。其中，参数 Fields 可以设置索引字段的属性，可设置的属性如下：

| 参数名 | 类型    | 说明                                  |
| ------ | ------- | ------------------------------------- |
| Type   | string  | 索引字段在 Elasticsearch 的映射类型，可选取值包括："text"、"keyword"、"wildcard"、"integer"、"long"、"float"、"double"、"date"、"boolean"<br> 在不指定该参数的情况下，字段将按照“字段类型映射表”中的规则建立映射 |
| Index  | boolean | 索引字段是否在 Elasticsearch 建立索引信息，默认值为 true，表示在 Elasticsearch 建立索引信息 <br>取值为 false 时，对应的索引字段在 Elasticsearch 中仅存储，不建立索引信息，无法索引该字段 |

###字段类型映射表###

| SequoiaDB 字段类型 | Elasticsearch 默认映射类型 | SequoiaDB 是否支持检索该类型的字段 | 说明 |
| ------------------ | -------------------------- | ---------------------------------- | ---- |
| String | text | 是 | - |
| NumberInt | long | 是 | - | - |
| NumberLong | long | 是 | - |
| NumberDouble | double | 是 | - |
| Bool | boolean | 是 | - |
| Date | date | 是 | - |
| TimeStamp | date | 是 | - |
| Object | object | 是 | - |
| Array  | array | 是 | 在 Array 类型的字段上创建全文索引，需保证所有数组元素的数据类型一致，否则无法检索 |
| jsOID  | 无对应类型 | 否 | - |
| NumberDecimal  | 无对应类型 | 否 | - |
| MinKey  | 无对应类型 | 否 | - |
| MaxKey  | 无对应类型 | 否 | - |
| jsNull  | 无对应类型 | 否 | - |
| Regex  | 无对应类型 | 否 | - |
| BinData  | 无对应类型 | 否 | - |

##全文索引环境部署##

用户在使用全文索引之前，需要先完成 Elasticsearch 集群、SequoiaDB 集群及搜索引擎适配器的部署。

> **Note:**
>
> 由于不同集群间集合的 UniqueID、原始索引名等可能相同，会导致 Elasticsearch 中生成的索引名重复。因此建议每个 SequoiaDB 集群使用不同的 Elasticsearch 集群，避免集群混用造成的数据错误。

###部署 Elasticsearch 集群###

下述示例以 Elasticsearch 安装目录为 `/opt/elasticsearch-7.17.7`、对外服务的 http 端口为 9200 ，介绍部署步骤。

1. 在 [Elasticsearch 官网][es]下载 Elasticsearch 安装包

    > **Note:**
    >
    > SequoiaDB v3.6.1 以下版本适配的 Elasticsearch 版本为 6.8.5；SequoiaDB v3.6.1 及以上版本适配的 Elasticsearch 版本为 7.17.7。

2. 解压安装包

    ```lang-bash
    $ tar -xzf elasticsearch-7.17.7.tar.gz
    ```

3. 切换至 Elasticsearch 安装目录

    ```lang-bash
    $ cd /opt/elasticsearch-7.17.7
    ```

4. 修改 Elasticsearch 集群配置
   
    > **Note:**
    >
    > - 用户可根据实际需求参考 [Elasticsearch 官方文档][es_config]修改 Elasticsearch 集群配置。
    > - 在测试环境中，支持全默认配置启动，即默认启动单节点的 Elasticsearch 集群。在实际使用中，用户应配置多节点的 Elasticsearch 集群。

5. 启动 Elasticsearch

    ```lang-bash
    $ ./bin/elasticsearch -d
    ```

6. 查看是否启动成功

    ```lang-bash
    $ jps
    ```

    输出如下内容表示启动成功：

    ```lang-text
    10183 Elasticsearch
    ```

7. 查看 Elasticsearch 集群状态

    ```lang-bash
    $ curl -XGET "sdbserver:9200/"
    ``` 

###部署搜索引擎适配器###

SequoiaDB 集群包含的每一个数据节点均需要启动一个对应的适配器节点，二者需要运行在同一台主机上。下述示例以 SequoiaDB 安装目录为 `/opt/sequoiadb`、已存在的数据节点为 11820、11830、11840，介绍部署步骤。

1. 切换至 `conf` 目录

    ```lang-bash
    $ cd /opt/sequoiadb/conf
    ```

2. 在当前目录下创建 `sdbseadapter` 目录

    ```lang-bash
    $ mkdir sdbseadapter
    ```

3. 切换至 `sdbseadapter` 目录

    ```lang-bash
    $ cd sdbseadapter
    ```

4. 创建各适配器节点对应的端口目录

    ```lang-bash
    $ mkdir 11827 11837 11847
    ```

5. 将适配器的配置文件分别拷贝至各节点对应的目录中

    ```lang-bash
    $ cp ../samples/sdbseadapter.conf 11827
    $ cp ../samples/sdbseadapter.conf 11837
    $ cp ../samples/sdbseadapter.conf 11847
    ```

6.  分别修改上述配置文件，以适配器节点 11837 为例，对应的数据节点端口为 11830，配置内容如下：

     ```lang-ini
     svcname=11837
     datanodehost=sdbserver
     datasvcname=11830
     searchenginehost=sdbserver
     searchengineport=9200
     diaglevel=3
     optimeout=30000
     bulkbuffsize=10
     logpath=/opt/sequoiadb/sdbadapterlog/11837
     ```

7. 启动各节点对应的适配器

    ```lang-bash
    $ sdbseactl -m start -p 11827,11837,11847
    ```

8. 查看所有适配器进程是否均已启动成功

    ```lang-bash
    $ sdbseactl -m list -l
    ```

    输出如下信息则启动成功：

    ```lang-text
    Name          SvcName      Role        PID       DataSvcName   Mode        StartTime
    sdbseadapter  11847        seadapter   29010     11840         read-write  2023-04-27-10.30.44 
    sdbseadapter  11837        seadapter   29013     11830         read-write  2023-04-27-10.30.44 
    sdbseadapter  11827        seadapter   29019     11820         read-only   2023-04-27-10.30.44 
    ```

##使用全文索引##

SequoiaDB 通过在查询中指定 Elasticsearch 的搜索条件，以进行全文检索。语法如下：  

```lang-javascript
find( { "": { "$Text": <search command> } } )
```

其中 \<search command\> 是 Elasticsearch 的搜索条件，需要使用 Elasticsearch 的 DSL（Domain Specific Language）语法。详情可参考 [Elasticsearch DSL][dsl] 官方文档。

###检索字符串类型的数据###

1. 创建集合 sample.employee

    ```lang-javascript
    > var cl = db.createCS("sample").createCL("employee")
    ```

2. 创建全文索引

    ```lang-javascript
    > cl.createIndex("idx_1", {"first_name": "text", "last_name": "text", "age": "text", "about": "text", "interests": "text"})
    ```

3. 将数据插入 sample.employee 中

    ```lang-javascript
    > cl.insert({"first_name": "John", "last_name": "Smith", "age": 25, "about": "I love to go rock climbing", "interests": ["sports", "music"]})
    > cl.insert({"first_name": "Jane", "last_name": "Smith", "age": 32, "about": "I like to collect rock albums", "interests": ["music"]})
    > cl.insert({"first_name": "Douglas", "last_name": "Fir", "age": 35, "about": "I like to build cabinets", "interests": ["forestry"]})
    ```

4. 使用全文索引模糊查询集合中 about 字段包含"rock climbing"的记录

    ```lang-javascript
    > cl.find({"": {"$Text": {"query": {"match": {"about": "rock climbing"}}}}}).hint({"": "idx_1"})
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

###检索日期类型的数据###

1. 创建集合 sample.employee

    ```lang-javascript
    > var cl = db.createCS("sample").createCL("employee")
    ```

2. 创建全文索引

    ```lang-javascript
    > cl.createIndex("timeidx", {"work_info.entry_time": "text"}, {Mappings: {"Fields": {"work_info.entry_time": {"Type": "date"}}}})
    ```

3. 将数据插入 sample.employee 中

    ```lang-javascript
    > cl.insert({"name": "John", "date_of_birth": {$date: "1998-03-05"}, "phone": 6883643, "work_info": {"entry_time": {$date: "2021-07-01"}, "post": "Engineer"}})
    > cl.insert({"name": "Jane", "date_of_birth": {$date: "1985-09-23"}, "phone": 8891638, "work_info": {"entry_time": {$date: "2017-08-21"}, "post": "Manager"}})
    > cl.insert({"name": "Douglas", "date_of_birth": {$date: "1991-08-16"}, "phone": 8390328, "work_info": {"entry_time": {$date: "2021-09-25"}, "post": "Engineer"}})
    ```

4. 使用全文索引查询集合中 work_info.entry_time 字段为"2021-07-01"的记录

    ```lang-javascript
    > cl.find({"": {"$Text": {"query": {"match": {"work_info.entry_time": "2021-07-01"}}}}}).hint({"": "timeidx"})
    {
      "_id": {
        "$oid": "6444f489f64323e18fc60230"
      },
      "name": "John",
      "date_of_birth": {
        "$date": "1998-03-05"
      },
      "phone": 6883643,
      "work_info": {
        "entry_time": {
          "$date": "2021-07-01"
        },
        "post": "Engineer"
      }
    }
    Return 1 row(s).
    ```

###检索嵌套字段###

1. 创建集合 sample.employee

    ```lang-javascript
    > var cl = db.createCS("sample").createCL("employee")
    ```

2. 创建全文索引

    ```lang-javascript
    > cl.createIndex("infoidx", {"work_info": "text"})
    ```

3. 将数据插入 sample.employee 中

    ```lang-javascript
    > cl.insert({"name": "John", "date_of_birth": {$date: "1998-03-05"}, "phone": 6883643, "work_info": {"entry_time": {$date: "2021-07-01"}, "post": "Engineer"}})
    > cl.insert({"name": "Jane", "date_of_birth": {$date: "1985-09-23"}, "phone": 8891638, "work_info": {"entry_time": {$date: "2017-08-21"}, "post": "Manager"}})
    > cl.insert({"name": "Douglas", "date_of_birth": {$date: "1991-08-16"}, "phone": 8390328, "work_info": {"entry_time": {$date: "2021-09-25"}, "post": "Engineer"}})
    ```

4. 执行查询

    使用全文索引查询集合中 work_info.post 字段为"Manager"的记录

    ```lang-javascript
    > cl.find({"": {"$Text": {"query": {"match": {"work_info.post": "Manager"}}}}}).hint({"": "infoidx"})
    {
      "_id": {
        "$oid": "6444f489f64323e18fc60231"
      },
      "name": "Jane",
      "date_of_birth": {
        "$date": "1985-09-23"
      },
      "phone": 8891638,
      "work_info": {
        "entry_time": {
          "$date": "2017-08-21"
        },
        "post": "Manager"
      }
    }
    Return 1 row(s).
    ```

    在 object 类型的字段上建立全文索引时，如果嵌套字段中包含时间字段，该时间字段将无法被检索

    ```lang-javascript
    > cl.find({"": {"$Text": {"query": {"match": {"work_info.entry_time":'2021-07-01'}}}}}).hint({"": "infoidx"})
    Return 0 row(s).
    ```

##参考##

更多操作可参考

| 操作 | 说明 |
| ---- | ---- |
| [query.explain()][explain] | 获取查询的访问计划 |
| [SdbCollection.dropIndex()][drop_index] | 删除指定的全文索引 |
| [SdbCollection.createIndex()][create_index] | 创建全文索引 |
| [SdbCollection.createIndexAsync()][create_index_async] | 异步创建全文索引 |




[^_^]:
     本文使用的所有链接和引用
[es]:http://www.elastic.co
[mapping-types]:https://www.elastic.co/guide/en/elasticsearch/reference/6.2/mapping-types.html
[es_config]:https://www.elastic.co/guide/en/elasticsearch/reference/6.8/settings.html
[regex]:manual/Manual/Operator/Match_Operator/regex.md
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[dsl]:https://elasticsearch-dsl.readthedocs.io/en/6.2.1/
[drop_index]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md
[full_text_search_flow]:images/Distributed_Engine/Architecture/Data_Model/full_text_search_flow.png
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[create_index]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[create_index_async]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndexAsync.md
