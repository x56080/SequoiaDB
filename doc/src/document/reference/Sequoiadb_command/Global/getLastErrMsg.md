##语法##
***getLastErrMsg()***

获取当前操作的详细错误信息。

##参数描述##

无

##返回值##

当存在错误信息时，返回为string，当不存在错误信息时，无返回值（即为void）

##版本信息##
2.6及以上版本

##示例##

* 获取当前操作的详细错误信息

  ```lang-javascript
  > db = new Sdb()
  (nofile):0 uncaught exception: -15
  > getLastErrMsg()
  Network error
  ```
