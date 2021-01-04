
##语法##
***db.collectionspace.collection.putLob\(\<file path\>, [oid]\)***

在集合中插入大对象。

## 参数描述##
| 参数名    | 参数类型 | 描述     | 是否必填 |
| --------- | -------- | -------- | -------- |
| file path | string   | 待上传的文件全路径。 | 是 |
| oid       | string   | 指定oid。 | 否 |

> **Note:**
>
> * 上传大对象成功后会返回其 oid。
> * 需要拥有文件的读权限。


##返回值##

成功返回oid，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。
##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 示例##

* 创建集合空间与集合

 ```lang-javascript
 > db.createCS('sample' )
 > db.sample.createCL('employee')
 ```

* 上传大对象文件

 ```lang-javascript
 > db.sample.employee.putLob('/opt/mylob')
 ```

* 上传指定oid大对象文件

 ```lang-javascript
 > db.sample.employee.putLob('/opt/mylob', '5bf3a024ed9954d596420256')
 ```