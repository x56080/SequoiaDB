[^_^]:
    备份恢复Readme
    作者：陈子川
    时间：20190301
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190401


在分布式数据库集群环境下，多副本机制可以有效避免集群单机的宕机风险，并提供高可用的容灾技术需求。而数据库备份功能，能够在用户误操作造成数据丢失后，帮助用户快速恢复原有数据库数据。

数据库备份功能，是对数据库现有数据进行备份。数据库备份功能可以帮助用户在以下场景快速恢复数据库原有数据:

* 数据库集群所有服务器损坏，并且无法修复
* 所有副本节点的磁盘数据损坏，并且无法恢复
* 用户误操作，导致删除或者修改了关键数据

SequoiaDB 巨杉数据库支持多种备份方法，其中包括:

* [全量备份][regular_bar_all]
* [增量备份][regular_bar_in]
* [日志归档][log_archive]

用户可以通过后续章节获取详细的说明。

[^_^]:
    TODO:该页面需要调整
    # Markdown Todo
    - [ ] 设置regular_bar_allo变量参数
    - [ ] 设置regular_bar_in变量参数

[^_^]:
    本文使用到的所有链接及引用。
[regular_bar_all]:manual/Distributed_Engine/Maintainance/Backup_Recovery/regular_bar.md#全量备份
[regular_bar_in]:manual/Distributed_Engine/Maintainance/Backup_Recovery/regular_bar.md#增量备份
[log_archive]:manual/Distributed_Engine/Maintainance/Backup_Recovery/log_archive.md
