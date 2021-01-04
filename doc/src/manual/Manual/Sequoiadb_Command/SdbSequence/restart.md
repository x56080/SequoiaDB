##名称##

restart - 重置序列

##语法##

**sequence.restart\(\)**

##描述##

当用户需要序列从起始值开始重新计数时，可以使用该函数重置序列。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

重置当前值为 10 的序列

```lang-javascript
> sequence.getNextValue()
10
> sequence.restart()
```

重置后，序列值回到起始值 1

```lang-javascript
> sequence.getNextValue()
1
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
