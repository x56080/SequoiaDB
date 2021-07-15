Cmd 类主要用于创建和操作 cmd 对象，包含的函数如下：

| 名称 | 描述 |
|------|------|
| Cmd() | 新建一个 Command 对象 |
| getCommand() | 获取上次执行的命令 |
| getInfo() | 获取 Command 对象的对象信息 |
| getLastOut() | 获取上次命令执行的返回结果 |
| getLastRet() | 获取上次命令执行返回的错误码 |
| run() | 执行 Shell 指令 |
| runJS() | 远程执行 JavaScript 代码 |
| start() | 后台执行 Shell 指令 |