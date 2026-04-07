##名称##

analyze - 指定运行模式为 analyze

##语法##

**diaglog.analyze([location]).path(\<collect filename\>)**

**diaglog.analyze([location]).path(\<collect filename\>).output(\<dir\>)**

##类别##

DiagLog

##描述##

设置运行模式为 analyze，分析收集回来的集群诊断日志，分析维度包含错误出现次数，错误出现时间。

##参数##

仅支持部分命令位置参数。

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| GroupID | number/array | 分区组 ID | 否 |
| GroupName | string/array | 分区组名 | 否 |
| NodeID | number/array | 节点 ID | 否 |
| HostName | string/array | 节点的主机名称 | 否 |
| ServiceName | string/array | 节点的服务名 | 否 |
| NodeName | string/array | 节点名称，格式为 \<HostName\>:\<svcname1\>[:svcname2...] ,<br> 如：sdbserver:11820:11830 | 否 |
| Role | string/array | 指定命令运行的节点角色，取值如下：<br>  "data"：数据节点<br>  "catalog"：编目节点<br>  "coord"：协调节点<br>  "all"：所有节点 | 否 |

##返回值##

目录名，目录下为分析后的结果。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v5.8 及以上版本

##示例##

* 新建一个 Sdb 对象

    ```lang-javascript
    > var db = new Sdb()
    ```

* 新建一个 DiagLog 对象

    ```lang-javascript
    > var diaglog = new DiagLog()
    ```

* 搜索最近 10 条报错 -79 错误的日志，并且把涉及的日志文件取回本地。

    ```lang-javascript
    > var fileName = diaglog.collect().error( -79 ).limit( 10 ).conn(db).run()
    ```

* 分析取回的文件。

    ```lang-javascript
    > diaglog.analyze().path( fileName )
    /tmp/sequoiadb/analyze/diaglog_2025-01-01-12:01:01.000.auto
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md