##语法##

***domain.addGroups( \<options\> )***

修改域的属性，为域添加组。

##参数描述##

*   `options` ( *Object*，*必填* )

    需要修改的属性列表。

    1.  `Groups`：新加的复制组。

        格式：`Groups : [ 'data1', 'data2' ]`

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

| 错误码 | 可能的原因   | 解决方法              |
| ------ | ------------ | --------------------- |
| -154   | 分区组不存在 | 使用列表查看分区组是否存在 |

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 首先创建一个域，包含两个复制组，开启自动切分

```lang-javascript
> var domain = db.createDomain( 'mydomain', ['data1', 'data2'], { AutoSplit: true } )
```

* 从域中添加另一个复制组 data3

```lang-javascript
> domain.addGroups( { Groups: ['data3'] } )
```
