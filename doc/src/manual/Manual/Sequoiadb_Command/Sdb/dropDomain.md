
##语法##
***db.dropDomain( \<name\> )***

删除指定域。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 域名 | 是 |

> **Note:**
>
> * dropDomain() 方法的定义格式必须指定 name 参数，并且 name 的值在系统中存在，否则操作异常。
> * 删除域前必须保证域中不存在任何数据。
> * 不能删除系统域SYSDOMAIN。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 删除一个之前创建的域。

 ```lang-javascript
 > db.dropDomain( 'mydomain' )
 ```

* 删除一个包含集合空间的域，返回错误：

 ```lang-javascript
 > db.dropDomain( 'hello' )
 (nofile):0 uncaught exception: -256
 > getLastErrMsg( -256 )
 Domain is not empty
 ```
