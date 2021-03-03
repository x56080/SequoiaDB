##语法##
***db.cancelTask( \<id\>, [isAsync] )***

取消任务。

##参数描述##

| 参数名 | 参数类型 | 描述   | 是否必填 |
|--------|----------|--------|----------|
| id     | 整数     |任务ID  | 是       |
| isAsync| 布尔     |是否异步| 否       |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

**停止切分任务**

```lang-javascript
> var taskid1 = db.foo.bar.splitAsync( "group1", "group2", 50 );
> db.cancelTask( taskid1, true )
```