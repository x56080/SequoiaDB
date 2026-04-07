##名称##

diaglog - 新建一个 DiagLog 对象

##语法##

**var diaglog = new DiagLog()**

##类别##

DiagLog

##描述##

该函数用于新建一个 DiagLog 对象。

##参数##

无

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v5.8 及以上版本

##示例##

1. 新建一个 DiagLog 对象。

	```lang-javascript
 	> var diaglog = new DiagLog()
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


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md