##名称##

putLob - 在集合中插入大对象

##语法##

**db.collectionspace.collection.putLob\(\<filepath\>, [oid]\)**

##类别##

SdbCollection

##描述##

该函数用于在集合中插入大对象。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| filepath | string | 待上传的本地文件全路径 | 是 |
| oid | string | 大对象的唯一标识 | 否 |

##返回值##

函数执行成功时，将返回一个 String 类型的 oid 字符串。用户可通过 oid 对大对象进行相关操作。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

- 将文件 `mylob.txt` 以大对象形式插入集合 sample.employee 中，并指定大对象 oid

    ```lang-javascript
    > db.sample.employee.putLob('/opt/mylob/mylob.txt', '5bf3a024ed9954d596420256')
    5bf3a024ed9954d596420256
    ```

    >**Note:**
    >
    > 用户可通过 [createLobID()][createLobID] 创建大对象 oid

- 将文件 `mylob.txt` 以大对象形式插入集合 sample.employee 中，不指定大对象 oid

    ```lang-javascript
    > db.sample.employee.putLob('/opt/mylob/mylob.txt')
    0000604f989a390002db009e
    ```