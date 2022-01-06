##名称##

updateConf - 修改 STP 节点的配置

##语法##

**stp.updateConf(\<config\>)**

##类别##

Stp

##描述##

该函数用于更新 STP 节点配置，并使新配置动态生效。

##参数##

config（ *object，必填* ）

需要修改的配置参数，具体配置项可参考 [STP 配置][stp]

格式：`{syncinterval: 100, diaglevel: 3}`

> **Note:**
>
> * 动态生效的配置在更改后，会更新至配置文件，成为固定的配置；禁止修改的配置在更改后，将会返回错误信息。
> * 用户可以通过 [stp.getConf()][getConf] 获取指定 STP 节点的配置信息。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v5.0 及以上版本

##示例##

修改 STP 节点的配置

```lang-javascript
> var stp = new Stp()
> stp.updateConf({diaglevel: 5, syncinterval: 100})
```

[^_^]:
    本文使用的所有引用及链接
[stp]:manual/Distributed_Engine/Architecture/Stp/Tools/stp.md
[getConf]:manual/Manual/Sequoiadb_Command/Stp/getConf.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
