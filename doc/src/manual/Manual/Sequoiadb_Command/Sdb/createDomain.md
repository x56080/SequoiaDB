
##语法##
***db.createDomain( \<name\>, \<groups\>, [options] )***

创建一个域。域中可以包含若干个复制组（Replica Group）。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 域名，全局唯一。 | 是 |
| groups | Json 数组 | 域包含的复制组。 | 否 |
| options | Json 对象 | 在创建域时可以通过 options 设置其他属性。 | 否 |

##格式##

目前通过 options 可设置域的属性有：

| 属性名 | 描述 | 格式 |
| ------ | ------ | ------ |
| AutoSplit | 是否自动切分散列分区集合 | AutoSplit: true |

> **Note:**
>
> * AutoSplit 只作用于散列分区集合。
> * 不能在空域（不包含复制组）上创建集合空间。

##返回值##

返回新建域的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 创建一个域，包含两个复制组。

 ```lang-javascript
 > db.createDomain( 'mydomain', [ 'group1', 'group2' ] )
 ```

* 创建一个域，包含两个复制组，并且指定自动切分。

 ```lang-javascript
 > db.createDomain( 'mydomain', [ 'group1', 'group2' ], { AutoSplit: true } )
 ```
