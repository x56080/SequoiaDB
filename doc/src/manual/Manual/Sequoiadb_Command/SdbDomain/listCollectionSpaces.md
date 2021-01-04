
##语法##
***domain.listCollectionSpaces()***

获取指定域中所有的集合空间。

##参数描述##

无

##返回值##

返回指定域下的集合空间信息，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。


##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 获取指定域下的集合空间信息

 ```lang-javascript
  > domain.listCollectionSpaces()
  {
      "Name": "sample" 
  }
 ```

