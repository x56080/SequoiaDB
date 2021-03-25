
##语法##

***File.find( \<options\>, \[filter\] )***

##类别##

File

##描述##

查找文件

##参数##

| 参数名    | 参数类型 | 默认值           | 描述                       | 是否必填 |
| --------- | -------- | ---------------- | -------------------------- | -------- |
| options   | JSON     | ---              | 查找模式和查找内容         | 是       |
| filter    | JSON     | 默认显示全部内容 | 筛选条件                   | 否       |

options 参数详细说明如下：

| 属性     | 值类型 | 描述 | 是否<br>必填 |
| -------- | ------ | ---- | ------------ |
| mode     | char   | { mode: 'n' } 表示根据文件名( filename )查找文件<br>{ mode: 'u' } 表示根据用户名( username )查找文件<br>{ mode: 'g' } 表示根据用户组名( groupname )查找文件<br>{ mode: 'p' } 表示根据权限( perssion )查找文件 | 是 |
| pathname | string | 指定查找的文件路径，默认为当前目录 | 否 |
| value    | string | 查找的内容                         | 是 |

filter 参数支持对结果中的某些字段进行 and 、 or 、not 和精确匹配计算，对结果集进行筛选。

##返回值##

返回查找内容。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##示例##

* 查找文件；

  ```lang-javascript
  > File.find( { mode: 'n', value: "file.txt", pathname: "/opt" } )
  {
      "pathname": "/opt/sequoiadb1/file.txt"
  }
  {
      "pathname": "/opt/sequoiadb2/file.txt"
  }
  {
      "pathname": "/opt/sequoiadb3/file.txt"
  }
  ```

* 查找文件后，对结果进行筛选。

 ```lang-javascript
 > File.find( { mode: 'n', value: "file.txt", pathname: "/opt" }, { $or: [ { pathname: "/opt/sequoiadb1/file.txt" }, { pathname: "/opt/sequoiadb2/file.txt" } ] } )
  {
      "pathname": "/opt/sequoiadb1/file.txt"
  }
  {
      "pathname": "/opt/sequoiadb2/file.txt"
  }
 ```
