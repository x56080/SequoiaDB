##语法##
***db.collectionspace.collection.putLob\(\<file path\>\)***

在集合中插入大对象。

## 参数描述##
| 参数名    | 参数类型 | 描述     | 是否必填 |
| --------- | -------- | -------- | -------- |
| file path | string   | 待上传的文件全路径。 | 是 |

> **Note:**
>
> * 上传大对象成功后会返回其 OID。
> * 需要拥有文件的读权限。


##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。
##错误##

[错误码](reference/Sequoiadb_error_code.md)

## 示例##

* 创建集合空间与集合

 ```lang-javascript
 > db.createCS('foo' )
 > db.foo.createCL('bar')
 ```

* 上传大对象文件

 ```lang-javascript
 > db.foo.bar.putLob('/opt/mylob')
 ```
