##名称##

forceStepUp - 强制将备节点升级为主节点

##语法##

**db.forceStepUp( [options] )**

##类别##

Sdb

##描述##

该函数用于在一个不具备选举条件的复制组中，强制将一个备节点升级为主节点。
**请谨慎使用该命令！**

##参数##

|参数名    |参数类型    |描述         |是否必填|
|--------- |----------- |------------ |----------|
|options   |Json 对象   |参数集合   |否|

   1. **options 选项**

  |参数名    |参数类型   |描述                           |默认值|
  |--------- |---------- |------------------------------ |--------|
  |Seconds   |int        |强制升级为主节点的持续时间   |120|

> **Note:**
>
> * 目前仅在 catalog 组中开放此功能。
> * 目标复制组中不能存在主节点，且其他节点的 LSN 不能比目标节点的 LSN 大。
> * 当持续时间到期，所有节点会重新按照选举规则进行选举。
> * 如果创建了用户，无法直接连接 catalog 节点。可以先修改 catalog 节点的配置文件中的 auth 参数，配置 auth=false 关闭 catalog 的鉴权功能，重启集群后再进行操作。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

连接 catalog 节点（hostname1:30000），并使其强制升主，持续300s。

```lang-javascript
> var db = new Sdb( "hostname1", 30000 ) ;
> db.forceStepUp( { Seconds: 300 } );
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md