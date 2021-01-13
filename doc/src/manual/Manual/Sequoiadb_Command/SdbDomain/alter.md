
##语法##

***domain.alter( \<options\> )***

修改域的属性。

请参考 [domain.setAttributes\(\)](manual/Manual/Sequoiadb_Command/SdbDomain/setAttributes.md)

##参数描述##

*   `options` ( *Object*，*必填* )

    需要修改的属性列表。

    1.  `Groups`：包含的复制组。

        格式：`Groups : [ 'data1', 'data2' ]`

    2.  `AutoSplit`：自动切分。

        格式：`AutoSplit : true | false`

> **Note:**
>
> * 删除复制组前必须保证其不包含任何数据。
> * AutoSplit 的更改不对之前创建的集合和集合空间产生影响。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

| 错误码 | 可能的原因   | 解决方法              |
| ------ | ------------ | --------------------- |
| -154   | 分区组不存在 | 使用列表查看分区组是否存在 |
| -215   | 域已经在     | 使用listDomains()查看域是否存在 |
| -256   | 域已被使用   | 使用listCollections()查看域是否存在集合 |

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 首先创建一个域，包含两个复制组，开启自动切分

```lang-javascript
> var domain = db.createDomain( 'mydomain', ['data1', 'data2'], { AutoSplit: true } )
```

* 从域中删除一个复制组 data2，添加另一个复制组 data3，最后域中包含 data1 和 data3 两个复制组

```lang-javascript
> domain.alter( { Groups: ['data1', 'data3'] } )
```

* 首先创建一个域，包含一个复制组，复制组中包含表 sample.employee。

```lang-javascript
> var domain = db.createDomain( 'mydomain', ['group1'] )
```

* 从域中删除原复制组，添加另一个复制组，将因把拥有数据的 group1 从域中删除而报错

```lang-javascript
> domain.alter( { Groups: ['group2'] } )
(nofile):0 uncaught exception: -256
Domain is not empty
```
