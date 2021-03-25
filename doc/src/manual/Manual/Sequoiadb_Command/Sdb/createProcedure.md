##名称##

createProcedure - 创建存储过程

##语法##

**db.createProcedure( \<code\> )**

##类别##

Sdb

##描述##

此函数用于在数据库对象中创建存储过程。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| code | 自定义函数 | 标准函数定义，不是字符串类型，在输入参数时不能使用引号。 | 是 |


> **Note：**
>
> * 推荐直接使用存储过程中已初始化全局的 db，且全局 db 采用当前执行该存储过程的会话的鉴权信息，如：`db.createProcedure( function getAll() { return db.sample.employee.find(); } )` 。
> * 自己初始化 db 的形式为 `var db = new Sdb()`，db 采用当前执行该存储过程的会话的鉴权信息。如果需要加入其它用户名和密码，为 `var db = new Sdb( 'usrname','passwd' )` 。这里需要注意的是，存储过程只能运行在已连接上的 db，不提供远程连接其他 db 的方法。在不需要鉴权的情况下，即使如 `var db = new Sdb( 'hostname', 'servicename' )` 语句正常执行。得到的 db 仍然是本地 db。
> * db 角色必须为协调节点。standalone 模式不提供存储过程功能。


##自定义函数##

* 函数定义

 （1） 函数必须包含函数名，不能使用如：function(x,y) { return x+y; }。

 （2） 在函数定义时可以调用其他函数甚至是不存在的函数，但需要保证运行时所有函数已存在。

 （3） 函数名全局唯一，不提供重载。

 （4） 每个函数均在全系统可用，随意删除一个存储过程可能导致他人运行失败。

* 函数参数

  ```lang-bash
  native type of JS
  ```

* 函数输出

 函数中所有标准输出，标准错误会被屏蔽。同时不建议在函数定义或执行时加入输出语句，大量的输出可能会导致存储过程运行失败。

* 函数返回值

 函数返回值可以是除 db 以外任意类型数据，如：function getCL() { return db.sample.employee; }。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 创建 sum 函数

 ```lang-javascript
 > db.createProcedure( function sum(x,y) { return x+y; } )
 ```

 创建之后可以使用 [db.listProcedures()](manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md) 查看函数信息。

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md