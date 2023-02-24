##名称##

shrinkSpace - 回收空闲空间

##语法##

**db.shrinkSpace([options])**

##类别##

Sdb

##描述##

该函数用于回收指定集合空间下所有文件的[空闲空间][shrinkSpace]。

##参数##

options（ *object，选填* ）

通过参数 options 可以指定待回收的集合空间和命令位置参数：

- CollectionSpace（ *string* ）：集合空间名称

    如果不指定该参数，默认回收所有集合空间下的空闲空间。

    格式：`CollectionSpace: "sample"`

-  Location Elements（ *object* ）：[命令位置参数][location]

    如果不指定该参数，默认在全局执行。

    格式：`GroupName: "db1"`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`shrinkSpace()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -34    | SDB_DMS_CS_NOTEXIST | 集合空间不存在 | 检查指定的集合空间是否存在 |
| -396   | SDB_SYSENV_NOT_SUPPORT | 系统环境不支持构造文件空洞 | 更换 ext4(Linux 3.0 或以上) 文件系统 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.4 及以上版本

##示例##

回收集合空间 sample 下的空闲空间

```lang-javascript
> db.shrinkSpace({CollectionSpace: "sample"})
```

[^_^]:
     本文使用的所有引用及链接
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[shrinkSpace]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md#空间回收