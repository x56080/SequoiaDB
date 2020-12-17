##语法##
***db.createCS( \<name\>, [options] )***

在数据库对象中创建集合空间。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 集合空间名，同一个数据库对象中，集合空间名必须唯一。 | 是 |
| options | Json | Json对象，集合空间可选属性。 | 否 |

 1. **options 格式**

 | 属性名 | 描述 | 格式 |
 | ------ | ------ | ------ |
 | PageSize | 数据页/索引页大小。单位为字节，默认值65536。 | PageSize: \<int32\> |
 | Domain | 所属域。默认值系统域 SYSDOMAIN，SYSDOMAIN 包含所有的复制组。 | Domain: \<string\> |
 | LobPageSize | Lob数据页大小。单位为字节，默认值262144。 | LobPageSize: \<int32\> |

> **Note:**
>
> * 集合空间名限制请参考[限制](reference/Sequoiadb_limitation.md)
> * 同一个数据库对象集合空间名必须唯一。
> * 在创建集合空间时用户可以指定数据页大小，指定后不可更改。如果不指定默认为65536B。
> * PageSize 只能选填0，4096，8192，16384，32768，65536之一，0即为默认值65536。
> * 所属域必须已经存在，且不能指定为系统域 SYSDOMAIN。
> * 为兼容较早版本接口，db.createCS( \<name\>, [PageSize] ) 同样可以工作。
> * LobPageSize只能选填0，4096，8192，16384，32768，65536，131072，262144，524288之一，0即为默认值262144。

##返回值##

返回新建集合空间的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 创建名为 foo 的集合空间，不指定数据页大小，即数据页大小为默认值65536B

 ```lang-javascript
 > db.createCS( "foo" )
 ```


* 创建名为 foo 的集合空间，指定数据页大小为4096B，所属域为“mydomain”

 ```lang-javascript
 > db.createCS( "foo", { PageSize: 4096, Domain: "mydomain" } )
 ```
