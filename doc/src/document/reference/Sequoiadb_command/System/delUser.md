##名称##

delUser - 删除操作系统用户

##语法##

**System.delUser(\<users\>)**

##类别##

System

##描述##

该函数用于删除操作系统用户。

##参数##

users（ *object，必填* ）

通过参数 users 可以设置需要删除的用户：

- name（ *string* ）：用户名，该参数必填

    格式：`name: "username"`

- isRemoveDir（ *boolean* ）：是否删除用户目录，默认为 false

    格式：`isRemoveDir: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

删除指定的系统用户。

```lang-javascript
> System.delUser({name: "newUser"})
```