[^_^]:
    操作指南-索引 Readme

索引是一种提高数据访问效率的特殊对象。在没有索引的情况下，精准查询少量数据需要扫描集合中的所有记录，该查询方式显然效率较低。如果存在索引，SequoiaDB 巨杉数据库可以通过特定字段的值快速定位到目标记录，极大地提升查询效率。

SequoiaDB 根据不同的查询需求，提供了不同类型的索引，主要包括：

- [单字段索引][single_field_index]
- [复合索引][compound_index]
- [唯一索引][unique_index]
- [全文索引][text_index]
- [独立索引][standalone_index]



[^_^]:
     本文使用的所有引用及链接
[single_field_index]:manual/Distributed_Engine/Operation/Index/single_field_index.md
[compound_index]:manual/Distributed_Engine/Operation/Index/compound_index.md
[unique_index]:manual/Distributed_Engine/Operation/Index/unique_index.md
[text_index]:manual/Distributed_Engine/Operation/Index/text_index.md
[standalone_index]:manual/Distributed_Engine/Operation/Index/standalone_index.md