Global 类作用于全局范围，包含的函数如下：

| 名称 | 描述 |
|------|------|
| help() |显示帮助信息 |
| print() |将输入内容输出到标准输出 |
| sleep() | 睡眠若干毫秒 |
| forceGC() | 强制 javascript 引擎回收已经释放的对象资源 |
| showClass() | 列举 sdb shell 内置的自定义类及内置的自定义类包含的所有方法 |
| getErr() | 获取错误码的描述信息 |
| getLastErrMsg() | 获取前一次操作的详细错误信息 |
| getLastErrObj() | 以 bson 对象的方式，返回前一次操作的详细错误信息 |
| getLastError() | 获取前一次操作返回的错误码 |
| setLastErrMsg() | 设置前一次操作的详细错误信息 |
| setLastErrObj() | 以 bson 对象的方式，设置前一次操作的详细错误信息 |
| setLastError() | 设置前一次操作返回的错误码 |
| getExePath() | 获取执行当前 js 脚本的程序的位置目录 |
| getRootPath() | 获取执行当前 js 脚本的程序的工作目录 |
| getSelfPath() | 获取当前执行的 js 脚本的位置目录 |
| import() | 导入执行指定的 js 文件 |
| importOnce() | 全局只导入执行一次指定的 js 文件 |
| jsonFormat() | 设置格式化打印 BSON |
| traceFmt() | 将 trace 文件格式化为用户可读的内容并输出到指定文件 |
