##名称##

diaglog - 新建一个 DiagLog 对象

##语法##

**var diaglog = new DiagLog( [hostname], [svcname] )**

**var diaglog = new DiagLog( [hostname], [svcname], [username], [password] )**

**var diaglog = new DiagLog( [hostname], [svcname], [CipherUser] )**

##类别##

DiagLog

##描述##

该函数用于新建一个 DiagLog 对象。

##参数##

| 参数名     | 参数类型 | 默认值             | 描述            | 是否必填 |
| ---------- | -------- | ------------------ | --------------- | -------- |
| hostname   | string   | localhost          | 主机名          | 否       |
| svcname    | int      | 11810              | 节点端口号      | 否       |
| username   | string   | 默认为空（''）     | 用户名          | 否       |
| password   | string   | 默认为空（''）     | 密码            | 否       |
| CipherUser | object   | ---                | [CipherUser][cipher] 对象 | 否       |

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v5.8 及以上版本

##示例##

1. 连接默认主机上的 SequoiaDB 集群，hostname 默认为 "localhost"，svcname 默认为 11810。

	```lang-javascript
 	> var diaglog = new DiagLog()
 	```

2. 连接指定机器上的 SequoiaDB 集群，目标机器 "sdbserver1"。

	```lang-javascript
 	> var diaglog = new DiagLog( "sdbserver1", 11810 )
	```

3. 使用用户名和密码连接指定机器上的 SequoiaDB 集群。

	```lang-javascript
 	> var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
	```

4. 使用 CipherUser 对象连接指定机器上的 SequoiaDB（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见[sdbpasswd][passwd]）。

   	```lang-javascript
    > var a = CipherUser( "sdbadmin" ).cipherFile( "/home/sdbadmin/passwd" )
 	>var diaglog = new DiagLog( "sdbserver1", 11810, a )
    ```

>**Note:**
>
> 用户使用 `DiagLog()` 函数搜索时，指定额外条件会影响搜索速度。
>
> 指定以下搜索条件时，函数内部可以优化搜索方法，可以加快搜索速度:
>
> - lastFile() 设置 search() 仅搜索最近的日志文件  
> - error() 设置 search() 中搜索的错误码  
> - diaglevel() 设置 search() 中过滤的日志级别  
> - keypattern() 设置 search() 中搜索的关键字  
> - pid() 设置 search() 中搜索的 pid  
> - tid() 设置 search() 中搜索的 tid  
> - limit() 限制 search() 返回的结果条数  
> - original() 设置 search() 返回的结果为原始日志格式  
>
> 指定以下搜索条件时，会使工具无法使用除 limit 以外的任何优化，搜索速度会很慢:
>
> - after() 设置 search() 结果上下文的下文条数  
> - before() 设置 search() 结果上下文的上文条数  
