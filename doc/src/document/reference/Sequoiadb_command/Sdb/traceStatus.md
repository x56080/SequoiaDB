##名称##

traceStatus - 查看当前程序跟踪的状态

##语法##

**db.traceStatus()**

##描述##

开启数据库引擎跟踪功能后，用户可使用该函数查看当前程序跟踪的状态。

##参数##

无

##返回值##

函数执行成功时，将通过游标（cursor）方式返回当前程序跟踪状态，返回的字段信息如下：

| 参数名        | 参数类型 | 参数描述         |
| --------------| ---------| -----------------|
| TraceStarted  | Boolean  | 跟踪是否开始 <br> "true"：跟踪开始 <br> "false"：跟踪未开始 |
| Wrapped       | Boolean  | 跟踪文件是否翻转 <br> "true"：已翻转 <br> "false"：未翻转   |
| Size          | Int64    | 跟踪文件大小     |
| FreeSize      | Int64    | 可用内存大小     |
| Mask          | String   | 所跟踪的模块，模块说明可参考 [SdbTraceOption](reference/Sequoiadb_command/AuxiliaryObjects/SdbTraceOption.md) 的 conponent 参数        |
| BreakPoint    | String   | 所跟踪的函数断点 |
| Threads       | Int32    | 线程号           |
| ThreadTypes   | String   | 线程类型，类型说明可参考 [SdbTraceOption](reference/Sequoiadb_command/AuxiliaryObjects/SdbTraceOption.md) 的 threadTypes 参数      |
| FunctionNames | String   | 所跟踪的函数名   |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v2.0 及以上版本

##示例##

* 开启数据库引擎程序跟踪的功能

   ```lang-javascript
   > db.traceOn( 100, new SdbTraceOption().components( "dms" ).functionNames( "_dmsStorageUnit::insertRecord"    ).threadTypes( "RestListener" ) )
   ```

* 查看当前程序跟踪的状态

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