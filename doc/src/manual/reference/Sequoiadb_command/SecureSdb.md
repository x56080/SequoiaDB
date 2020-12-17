##名称##

SecureSdb - SecureSdb 对象。

##语法##

***var securesdb = new SecureSdb( [hostname], [svcname] )***

***var securesdb = new SecureSdb( [hostname], [svcname], [username], [password] )***

##类别##

SecureSdb

##描述##

新建一个 SecureSdb 对象。

> **Note:**

> 1. SecureSdb 是 Sdb 的子类，SecureSdb 的对象使用 SSL 连接。

> 2. 在使用 SecureSdb 之前需要先设置数据库配置项 --usessl=true ，请参考[配置项参数](database_management/database_configuration/configuration_parameters.md)。

> 3. SecureSdb 对象和 Sdb 对象的方法和语法一致。 

> 4. 目前只有企业版支持SSL功能。

##参数##

| 参数名   | 参数类型 | 默认值            | 描述         | 是否必填 |
| -------- | -------- | ----------------- | ------------ | -------- |
| hostname | string   | localhost         | 主机 IP 地址 | 否     |
| svcname  | int      | 本地 coord 的端口 | coord 的端口 | 否     |
| username  | string      | 默认为空（''） | 用户名 | 否     |
| password  | string      | 默认为空（''）| 密码 | 否     |

##返回值##

成功：返回 SecureSdb 对象。 

失败：抛出异常。


##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 新建一个 SecureSdb 对象

   ```lang-javascript
   > var securesdb = new SecureSdb( "192.168.20.71", 11810 )
   ```