##名称##

truncateLob - 截断集合中的大对象

##语法##

**db.collectionspace.collection.truncateLob\(\<oid\>, \<length\>\)**

##类别##

SdbCollection

##描述##

该函数用于截断集合中的大对象。

##参数##

- oid（ *string，必填* ）

    大对象的唯一标识    

- length（ *number，必填* ）

    截断后大对象的长度

    - length 的取值小于 0 时，输出报错信息。
    - length 的取值大于大对象的长度时，不发生截断。


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

指定 oid 为"5435e7b69487faa663000897"的大对象，将其长度截断为 0

```lang-javascript
> db.sample.employee.truncateLob("5435e7b69487faa663000897", 0)
```
