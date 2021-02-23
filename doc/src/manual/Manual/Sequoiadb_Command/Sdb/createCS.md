##名称##

createCS - 创建集合空间

##语法##

**db.createCS( \<name\>, [options] )**

##类别##

Sdb

##描述##

该函数用于在数据库对象中创建集合空间。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 集合空间名，同一个数据库对象中，集合空间名必须唯一。 | 是 |
| options | Json | Json对象，集合空间可选属性。 | 否 |

**options 格式**

| 属性名 | 描述 | 格式 |
| ------ | ------ | ------ |
| PageSize | 数据页/索引页大小。单位为字节，默认值65536。 | PageSize: \<int32\> |
| Domain | 所属域。默认值系统域 SYSDOMAIN，SYSDOMAIN 包含所有的复制组。 | Domain: \<string\> |
| LobPageSize | Lob数据页大小。单位为字节，默认值262144。 | LobPageSize: \<int32\> |

> **Note:**
>
> * 集合空间名限制可以参考[限制][limitation]
> * 同一个数据库对象集合空间名必须唯一。
> * 在创建集合空间时用户可以指定数据页大小，指定后不可更改。如果不指定默认为65536B。
> * PageSize 只能选填0，4096，8192，16384，32768，65536之一，0即为默认值65536。
> * 所属域必须已经存在，且不能为系统域SYSDOMAIN。
> * 为兼容较早版本接口，db.createCS( \<name\>, [PageSize] ) 同样可以工作。
> * LobPageSize只能选填0，4096，8192，16384，32768，65536，131072，262144，524288之一，0即为默认值262144。

##返回值##

函数执行成功时，将返回一个 SdbCS 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 创建名为 sample 的集合空间，不指定数据页大小，即数据页大小为默认值65536B

 ```lang-javascript
 > db.createCS( "sample" )
 ```


* 创建名为 sample 的集合空间，指定数据页大小为4096B，所属域为“mydomain”

 ```lang-javascript
 > db.createCS( "sample", { PageSize: 4096, Domain: "mydomain" } )
 ```


[^_^]:
    本文使用的所有引用和链接
[limitation]:manual/Manual/sequoiadb_limitation.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md