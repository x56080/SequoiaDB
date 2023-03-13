[^_^]:
    Information Schema

SequoiaDB 巨杉数据库支持跨结构化、半结构化、非结构化的多模数据处理，应用程序可以通过不同的数据库实例向 SequoiaDB 进行数据的写入、读取等。在数据操作的过程中，所存储数据的结构会随着业务的需求而变化。为此 SequoiaDB 提供 Information Schema 功能，提升表结构的变更效率，保证数据库的读写性能。

##逻辑架构##

Information Schema 通过“模式”的转换实现表结构的快速变更，模式包含内部模式和外部模式。外部模式（Schema）能与数据库实例进行有效对接，实现结构信息的同步变更；内部模式（InfoSchema）能动态分析记录字段的结构，可快速判断数据的特定结构并校验结构变更的合法性，提升表结构的变更效率。其架构图如下：

![pc][pc]

对于已启用 Information Schema 的集合，外部模式仅存储于编目节点中；而内部模式与集合的结构有关，在集合所在的每一个分区上都存在对应的内部模式，用于存储该分区上数据的结构信息。

##参考##

具体操作可参考[使用 Information Schema][operation]。


[^_^]:
    本文使用的所有引用和链接
[pc]:images/Distributed_Engine/Architecture/information_schema.png
[sysschema]:manual/Manual/Catalog_Table/SYSDATASOURCES.md
[operation]:manual/Distributed_Engine/Operation/infoSchema.md