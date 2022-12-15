// 对 Sdb 的 Error 类型进行特殊处理，实现：
// 1. 特殊处理：new Error(new Error(0)) 输出为  Error: Error: 0，更改为输出 0
// 2. 特殊处理：e.message == errno，更改为可直接 e == errno
Error.prototype.toString = function(){  return this.message; }
