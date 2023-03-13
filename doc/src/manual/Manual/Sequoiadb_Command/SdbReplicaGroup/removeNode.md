##名称##

removeNode - 删除当前复制组中的指定节点

##语法##

**rg.removeNode(\<hostname\>, \<svcname\>, [options])**

##类别##

SdbReplicaGroup

##描述##

该函数用于删除当前复制组中的指定节点。

##参数##

- hostname（ *string，必填* ）

    主机名

- svcname（ *number/string，必填* ）

    节点端口号

- options（ *object，选填* ）

    通过 options 参数可以设置选填参数：

    Enforced（ *boolean* ）：是否强制删除节点，默认值为 false，表示不强制删除节点

    - 当复制组存在多个节点时，如果需要删除主节点，需指定 Enforced 为 true。
    - 如果需要删除组内唯一的空节点，需指定 Enforced 为 true。

    格式：`Enforced: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。 

##错误##

`removeNode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ---------|----------------| ---------|
| -204   | SDB_CATA_RM_NODE_FORBIDDEN | 尝试删除组内唯一的非空节点 | 需先移除当前节点的数据，再执行删除操作 |
| -206   | SDB_CATA_RM_CATA_FORBIDDEN | 尝试删除主编目节点 | 只能删除备编目节点 |
| -79    | SDB_NET_CANNOT_CONNECT | 删除节点主机上的CM进程不存在，或者主机宕机 | 如果需要强制删除，可以加入 {Enforced: true} 选项 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

- 删除复制组 group1 中的节点

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.removeNode("sdbserver", 11820)
    ```

- 强制删除复制组 group1 中的节点

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.removeNode("sdbserver", 11820, {Enforced: true})
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md