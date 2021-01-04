
##语法##

***domain.setGroups( \<options\> )***

修改域的属性，为域设置组。

##参数描述##

*   `options` ( *Object*，*必填* )

    需要修改的属性列表。

    1.  `Groups`：包含的复制组。

        格式：`Groups : [ 'data1', 'data2' ]`

> **Note:**
>
> * 删除复制组前必须保证其不包含任何数据。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。关于错误处理可以参考[常见错误处理指南][faq]。

##错误##

| 错误码 | 可能的原因   | 解决方法              |
| ------ | ------------ | --------------------- |
| -154   | 复制组不存在 | 使用列表查看复制组是否存在 |
| -256   | 域非空       | 使用listCollections()查看域是否存在集合 |

[错误码][error_code]

##示例##

* 首先创建一个域，包含两个复制组，开启自动切分

```lang-javascript
> var domain = db.createDomain( 'mydomain', ['data1', 'data2'], { AutoSplit: true } )
```

* 从域中删除一个复制组 data2，添加另一个复制组 data3，最后域中包含 data1 和 data3 两个复制组

```lang-javascript
> domain.setGroups( { Groups: ['data1', 'data3'] } )
```


[^_^]:
    本文使用的所有链接及引用
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/faq.md
[error_code]:manual/Manual/Sequoiadb_error_code.md