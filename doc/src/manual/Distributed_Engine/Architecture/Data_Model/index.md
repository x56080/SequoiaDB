[^_^]:
    索引

索引是一种提高数据访问效率的特殊对象。创建索引后，SequoiaDB 巨杉数据库可以通过索引查找特定值，以快速定位到匹配的记录，大大提升查询效率。

##逻辑架构##
           
创建索引时，数据库会将指定字段的值拷贝到一个数据结构索引项中，并对其进行排序。由于索引项是以 B 树的形式存储，通过树的遍历可以快速锁定满足条件的索引项，实现高效查询。

下述以集合中的 id 字段建立索引，通过索引查询 id=5 的记录为例，查询流程如图中红色线段所示：

![avatar][picture1]

1. 找到 id 字段对应的索引
2. 在索引中找到符合条件的索引项
3. 通过索引项中存储的位置信息找到完整的记录并返回
4. 查询完成

##索引类型##

SequoiaDB 提供不同类型的索引，以支持特定类型的数据和查询。

###单字段索引###

单字段索引是指根据记录中任一字段创建的索引。更多说明及使用可参考[单字段索引][single_field]。

###复合索引###

复合索引是指结合记录中多个字段创建的索引。如果用户在查询时经常使用某几个字段，可以为这些字段创建复合索引，使查询更加高效。更多说明及使用可参考[复合索引][compound]。

###唯一索引###

唯一索引用于保证索引字段值的唯一性。在使用唯一索引时，如果插入或更新的索引字段值在集合中已存在，则操作会报错。更多说明及使用可参考[唯一索引][unique]。

###全文索引###

全文索引用于在大量文本中进行快速检索。与其他索引相比，全文索引可以快速定位关键词出现的位置，以提升检索效率。更多说明及使用可参考[全文索引][text]。

###独立索引###

独立索引是指在集合的部分数据节点上单独创建的索引。如果用户希望只在指定的数据节点中使用索引，可以为这些节点创建独立索引。更多说明及使用可参考[独立索引][standalone]。

##参考##

一个集合可以拥有多个索引，一个索引也可以拥有多个字段。详细规格可参考数据库[限制][limit]。




[^_^]:
    本文使用的所有链接和引用
[picture1]:images/Distributed_Engine/Architecture/Data_Model/index_picture_1.png
[single_field]:manual/Distributed_Engine/Operation/Index/single_field_index.md
[compound]:manual/Distributed_Engine/Operation/Index/compound_index.md
[standalone]:manual/Distributed_Engine/Operation/Index/standalone_index.md
[text]:manual/Distributed_Engine/Operation/Index/text_index.md
[unique]:manual/Distributed_Engine/Operation/Index/unique_index.md
[limit]:manual/Manual/sequoiadb_limitation.md