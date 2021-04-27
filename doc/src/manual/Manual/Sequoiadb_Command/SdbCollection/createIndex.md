## 名称

createIndex - 创建索引

## 语法

**db.collectionspace.collection.createIndex\(\<name\>,\<indexDef\>,\[isUnique\],\[enforced\],\[sortBufferSize\])**

**db.collectionspace.collection.createIndex\(\<name\>,\<indexDef\>,\[options\])**

## 类别

SdbCollection

## 描述

该函数用于为集合创建[索引][index]，提高查询速度。

## 参数

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| name | string | 索引名，同一个集合中的索引名必须唯一。 | 是 |
| indexDef | Json 对象 |   索引键，包含一个或多个指定索引字段与类型的对象。类型值 1 表示字段升序，-1 表示字段降序，"text" 则表示创建[全文索引][text_index] | 是 |
| isUnique | Boolean | 索引是否唯一，默认 false。设置为 true 时代表该索引为唯一索引。 | 否 |
| enforced | Boolean | 索引是否强制唯一，可选参数，在 isUnique 为 true 时生效，默认 false。设置为 true 时代表该索引在 isUnique 为 true 的前提下，不可存在一个以上全空的索引键。 | 否 |
| sortBufferSize | int | 创建索引时使用的排序缓存的大小，单位为MB。取值为0时表示不使用排序缓存。默认为64。| 否 |
| options | Json 对象 | 可选项，详见 options 选项说明。| 否 |

options 选项：

| 属性名          | 参数类型 | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| Unique          | Boolean  | 索引是否唯一。| false  |
| Enforced        | Boolean  | 索引是否强制唯一。| false  |
| NotNull         | Boolean  | 索引的任意一个字段是否允许为 null 或者不存在。| false  |
| SortBufferSize  | int      | 创建索引时使用的排序缓存的大小。| 64MB  |
| NotArray        | Boolean  | 索引的任意一个字段是否允许为数组。| false |

> **Note:**
>
> * 在唯一索引所指定的索引键字段上，集合中不可存在一条以上的记录完全重复。
> * 索引名限制、索引字段的最大数量、索引键的最大长度请参考[限制][limitation]。
> * 在集合记录数据量较大时（大于1000万条记录）适当增大排序缓存大小可以提高创建索引的速度。
> * 对于全文索引，参数 isUnique、enforced 及 sortBufferSize 无意义。

## 返回值

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

## 版本

v2.0 及以上版本

## 示例

* 在集合 employee 下为字段名 age 创建名为 ageIndex 的唯一索引，记录按 age 字段值的升序排序。

    ```lang-javascript
    > db.sample.employee.createIndex( "ageIndex", { age: 1 }, true )
    ```

* 集合 employee 创建唯一索引，并且索引字段不允许为 null 或者不存在 。

    ```lang-javascript
    > db.sample.employee.createIndex( "ab", { a: 1, b: 1 }, { Unique: true, NotNull: true } )
    >
    > // b 字段为 null，插入索引时报错
    > db.sample.employee.insert( { a: 1, b: null } )
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    Takes 0.002531s.
    > 
    > // b 字段不存在，插入索引时报错
    > db.sample.employee.insert( { a: 1 } )
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    Takes 0.002531s.
    ```

* 在集合 employee 中的 address 及 tags 字段上建立[全文索引][text_index]，用于对这两个字段进行全文检索。

    ```lang-javascript
    > db.sample.employee.createIndex( "addr_tags", { address: "text", tags: "text" } )
    ```

* 集合 employee 创建索引，并且索引字段不允许为数组 。

    ```lang-javascript
    > db.sample.employee.createIndex( "ab", { a: 1, b: 1 }, { NotArray: true} )
    >
    > // a字段为数组，插入索引时报错
    > db.sample.employee.insert( { a: [1],b: 10 } )
    sdb.js:645 uncaught exception: -364
    Any field of index key cannot be array
    Takes 0.001760s.
    ```


[^_^]:
     本文使用的所有引用和链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[limitation]:manual/Manual/sequoiadb_limitation.md#索引
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[error_guide]:manual/FAQ/faq_sdb.md
