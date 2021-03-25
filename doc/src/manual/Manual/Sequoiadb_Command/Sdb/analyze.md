##名称##

analyze - 收集统计信息

##语法##

**db.analyze( [options] )**

##类别##

Sdb

##描述##

该函数用于分析集合和索引的数据，并收集统计信息。

##参数##

| 参数名 |参数类型| 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options |Json 对象| 设定 **分析模式**、 **指定集合空间** 以及 **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

1. **Options 格式**

| 属性名 | 描述 | 约束 | 格式 |
| ------ | ------ | ------ | ------ |
| Mode | 进行分析的模式，整数类型，(1-5)：<br/>1：进行抽样分析，生成统计信息<br/>2：进行全量数据分析，生成统计信息<br/>3：生成默认的统计信息<br/>4：加载统计信息到缓存中<br/>5：清除缓存的统计信息<br>默认值为 1 | Mode取 1 - 3 时，必须在主数据节点上执行</br>Mode取 3 时，需要指定集合全名。此时，生成的统计信息将包含集合及该集合上所有索引的统计信息；如果用户同时指定集合全名和该集合上具体的索引名，生成的统计信息将包含集合及该集合上指定索引的统计信息</br>Mode取 4，5 时，可以在备数据节点上执行 | Mode:1 |
| CollectionSpace | 指定需要分析的集合名称，字符串类型。默认值为空。 | 不能与Collection同时使用 | CollectionSpace:"sample" |
| Collection | 指定需要分析的集合名称，字符串类型。默认值为空。 | 不能与CollecitonSpace同时使用<br/>必须是Collection的全名 | Collection:"sample.employee" |
| Index | 指定需要分析的索引名称，字符串类型。默认值为空。 | 如果指定该参数，需要指定Collection参数 | Index:"index" |
| SampleNum | 指定抽样的数据个数，整数类型，范围为 100 - 10000 ，默认值为 200 | 不能与SamplePercent同时使用 | SampleNum:1000 |
| SamplePercent | 指定抽样的比例，浮点数类型，范围为：0.0 - 100.0 | 不能与SampleNum同时使用<br>集合数据个数和比例的乘积为抽样的数据个数，自动调整在 100 - 10000 之间（小于 100 调整为 100，大于 10000 调整为 10000）<br>缺省则不使用 SamplePercent，而选取 SampleNum 的默认值 200 | SamplePercent:50 |
| Location Elements | 命令位置参数项，详细见 **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | | GroupName:"db1" |

2. **统计信息**

统计信息的具体描述可以参考[统计信息](manual/Distributed_Engine/Maintainance/Access_Plan/statistics.md)一节。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`analyze()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -23 | SDB_DMS_NOTEXIST    | 指定的集合不存在。| 检查集合是否存在|
| -47 | SDB_IXM_NOTEXIST | 指定的索引不存在。 | 重试操作，若故障未修复，则需要联系售后工程师进行修复。|
| -6  | SDB_INVALIDARG | 指定的参数可能存在冲突，请参考 **Options** 的约束。| 查看对应节点的诊断日志，找到该参数错误的详细描述，并加以修正重试。|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.9 及以上版本

##示例##

- 对全系统所有集合空间进行统计信息分析和收集

  ```lang-javascript
  > db.analyze()
  ```

- 对指定集合空间"sample"进行统计信息分析和收集

  ```lang-javascript
  > db.analyze( { CollectionSpace : "sample" } )
  ```

- 对指定数据组"group1"进行统计信息分析和收集

  ```lang-javascript
  > db.analyze( { GroupName : "group1" } )
  ```

- 对指定集合"sample.employee"进行统计信息收集，并且指定Sample的数量

  ```lang-javascript
  > db.analyze( { Collection : "sample.employee", SampleNum : 1000 } )
  ```

- 对指定集合"sample.employee"的索引"index"进行统计信息收集

  ```lang-javascript
  > db.analyze( { Collection : "sample.employee", Index : "index" } )
  ```

- 对指定集合"sample.employee"生成清空统计信息缓存

  ```lang-javascript
  > db.analyze( { Collection : "sample.employee", Mode : 5 } )
  ```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md