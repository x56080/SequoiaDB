##语法##
***db.flushConfigure( \<rule\> )***

将配置刷盘至配置文件

##参数描述##

|参数名    |参数类型    |描述         |是否必填|
|--------- |----------- |------------ |----------|
|rule      |Json 对象   |刷盘规则     |是|

1. **rule 格式**

 |属性名   |  描述                       |                             格式|
 |---------| ----------------------------------------------------- |  --------|
 |Global   | true 表示将全系统配置刷盘，false 表示只将本节点配置刷盘  | Global: true|

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

- 刷盘数据库配置

 ```lang-javascript
 > db.flushConfigure( { Global: true } )
 ```
