##语法##

***db.traceStatus()***

##类别##

Sdb

##描述##

查看当前程序跟踪的状态。

##参数##

无

##返回值##

返回字段信息如下：

| 参数名        | 参数类型 | 参数描述         |
| --------------| ---------| -----------------|
| TraceStarted  | Boolean  | 跟踪是否开始 <br> "true"：跟踪开始 <br> "false"：跟踪未开始                                  |
| Wrapped       | Boolean  | 跟踪文件写至与指定文件大小相同时是否回写 <br> "true"：回写 <br> "false"：不回写             |
| Size          | Int64    | 跟踪文件大小     |
| FreeSize      | Int64    | 可用内存大小     |
| Mask          | String   | 所跟踪的模块     |
| BreakPoint    | String   | 所跟踪的函数断点 |
| Threads       | Int32    | 线程号           |
| ThreadTypes   | String   | 线程类型         |
| FunctionNames | String   | 所跟踪的函数名   |

出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 开启数据库引擎程序跟踪的功能

```lang-javascript
> db.traceOn( 100, new SdbTraceOption().components( "dms" ).functionNames( "_dmsStorageUnit::insertRecord" ).threadTypes( "RestListener" ) )
   ```

* 查看当前程序跟踪的状态：

```lang-javascript
> db.traceStatus()
{
  "TraceStarted": true,
  "Wrapped": false,
  "Size": 104857600,
  "FreeSize": 104857600,
  "PadSize": 0,
  "Mask": [
    "dms"
  ],
  "BreakPoint": [],
  "Threads": [],
  "ThreadTypes": [
    "RestListener"
  ],
  "FunctionNames": [
    "_dmsStorageUnit::insertRecord"
  ]
}
```