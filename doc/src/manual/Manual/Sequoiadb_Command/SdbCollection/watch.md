##名称##

watch - 订阅集合的变更流

##语法##

**SdbCollection.watch( [token], [options] )**

##类别##

SdbCollection

##描述##

该函数用于订阅集合的变更流。

##参数##

- token （ *StreamToken，选填* ）

    指定本次变更流的开始的位置

- options （ *Object，选填* ）

    - ChangeTypes（ *string* ）

        订阅的变更事件类型。支持的类型有 "RECORD"（BSON 记录的 DML 操作）, "DDL"（DDL操作）, "TRANS"（事务变更操作）, "ALL"（所有类型）。可以用 "|" 来连接多个类型，大小写不敏感。默认为："RECORD|DDL"。暂不支持 LOB 操作。

        格式：`ChangeTypes: "RECORD|DDL"`

    - MaxWaitTime（ *integer* ）

        指定没有新数据时最大等待的时间，单位：秒。超时后，变更流将返回 empty 的 control 类型记录。小于 0 为一直等待，0 为不等待。默认：1 秒。

        格式：`MaxWaitTime: 1`

    - CacheSize（ *integer* ）

        缓存日志的数据量，单位：MB。取值范围：0~2048，0 是不设置缓存，最大 2 GB。默认：32。超过缓存大小的日志将需要从日志文件中读取。

        格式：`CacheSize: 32`

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取返回的变更流数据。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`watch()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| --- | --- | --- | --- |
| -6 | SDB_INVALIDARG | 填写的参数错误，无效参数等，将返回详细的错误信息描述 | 填写正确的参数 |
| -150 | SDB_RTN_IN_REBUILD | 节点正在重建 | 需等全量同步结束才能开始订阅 |
| -129 | SDB_CLS_FULL_SYNC | 节点正在全量同步 | 需等全量同步结束才能开始订阅 |
| -23 | SDB_DMS_NOTEXIST | 指定订阅的集合不存在 | 检查集合是否存在 |
| -34 | SDB_DMS_CS_NOTEXIST | 指定订阅的集合空间不存在 | 检查集合空间是否存在 |
| -404 | SDB_STREAM_NOT_RESUMABLE | 变更流不可恢复执行，当前日志LSN和变更流恢复的LSN相差太远 | 考虑进行全量同步，或者改大节点的[配置参数][config] changestreamresumewindow |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v7.2.2 及以上版本

##示例##

```lang-javascript
// 创建StreamToken对象
var token = new StreamToken();

// 设置订阅选项
var options = {
  ChangeTypes: "RECORD|DDL",
  MaxWaitTime: 2,
  CacheSize: 64
};

// 订阅变更流
var cursor = db.foo.bar.watch(token, options);

// 获取并打印变更流数据
while (cursor.next()) {
  println(cursor.current());
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[config]:manual/Manual/Database_Configuration/configuration_parameters.md