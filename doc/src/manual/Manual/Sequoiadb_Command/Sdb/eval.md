
##语法##
***db.eval( \<code\> )***

根据需要填入 JavaScript 语句。同时可以在语句中调用已经创建好的存储过程。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| code | 字符串 | JavaScript 语句或者创建好的存储过程函数 | 是 |

> **Note：**
>
> 虽然语句中的所有输出都会被屏蔽，但还是建议不要加入任何打印语句。

##返回值##

（1） 执行成功则按照语句返回结果。可以将返回值直接赋值给另一个变量。如：`var a = db.eval( 'db.sample.employee' ); a.find(); `

（2） 执行失败会返回错误码及错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

（3） 在函数执行结束前操作不会返回。中途退出则终止整个执行，但已经执行的代码不会被回滚。

（4） 自定义返回值的长度有一定限制，参考 SequoiaDB 插入记录的最大长度。

（5） 支持定义临时函数。如：`db.eval( 'function sum(x,y){return x+y;} sum(1,2)' )`

（6） 全局 db 使用方式与 [createProcedure()](manual/Manual/Sequoiadb_Command/Sdb/createProcedure.md) 相同。

##示例##

* 在eval() 方法中调用存储过程函数 sum

 ```lang-javascript
 //初始时 sum() 方法不存在，返回异常信息
 > var a = db.eval( 'sum(1,2)' );
 { "errmsg": "(nofile):1 ReferenceError: getCL is not defined", "retCode": -152 }
(nofile):0 uncaught exception: -152
 //初始化 sum()
 > db.createProcedure( function sum(x,y){return x+y;} )
 //调用 sum()
 > db.eval( 'sum(1,2)' )
 3
 ```

* 在 eval() 方法中填写 JavaScript 语句

 ```lang-javascript
 > var rc = db.eval( "db.sample.employee" )
 > rc.find()
 {
   "_id": {
     "$oid": "5248d3867159ae144a000000"
   },
   "a": 1
 }
 {
   "_id": {
     "$oid": "5248d3897159ae144a000001"
   },
   "a": 2
 }...
 ```
