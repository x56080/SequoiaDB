##语法##
***getLastError()***

获取当前操作的错误码。

##参数描述##

无

##返回值##

int类型，0表示无错误，非0表示错误。

##版本信息##
2.6及以上版本

##示例##

* 获取当前操作的错误码

  ```lang-javascript
  > db = new Sdb()
  (nofile):0 uncaught exception: -15
  > getLastError()
  -15
  ```
