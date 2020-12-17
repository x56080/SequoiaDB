##语法##

***System.addUser( \<users\> )***

##类别##

System

##描述##

添加系统用户

##参数##

| 参数名  | 参数类型 | 默认值       | 描述             | 是否必填 |
| ------- | -------- | ------------ | ---------------- | -------- |
| users     | JSON   | ---          | 用户信息       | 是       |

users 参数详细说明如下：

| 属性     | 值类型 | 是否<br>必填 | 格式 | 描述 |
| -------- | ------ | -------- | -------------------- | ---------------------------------- |
| name    | string |     是   | { "name": newUser }     | 用户名                        |
| group    | string |     否   | { "group": groupname }     | 用户组                        |

> Note：

> group 参数必须是已存在的用户组，若不指定则默认创建与 name 参数同名的用户组

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 添加一个系统用户 

  ```lang-javascript
  > System.addUser( { "name": "newUser", "group": "root" } )
  ```