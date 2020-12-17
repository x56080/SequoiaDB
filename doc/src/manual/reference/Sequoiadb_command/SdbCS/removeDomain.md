##名称##

removeDomain - 移除集合空间的所属域。

##语法##

**db.collectionspace.removeDomain()**

##类别##

Collection Space

##描述##

移除集合空间的所属域。

##返回值##

成功：无。

失败：抛出异常。

##错误##

当异常抛出时，可以通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取[错误码](reference/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/faq.md)了解更多内容。

##版本##

v2.10及以上版本。

##示例##

1. 创建一个集合空间并指定一个域，然后把该集合空间移除该域

    ```lang-javascript
    > db.createCS( 'foo', { Domain : 'domain' } )
    > db.foo.removeDomain()
    ```
