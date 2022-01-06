[^_^]:
    SparkSQL 实例-数据类型映射表

本文档主要介绍存储类型与 SparkSQL 实例类型映射，以及 SequoiaDB 存储引擎向 SparkSQL 实例类型转换的兼容性。

##存储类型与 SparkSQL 实例类型映射##

| 存储引擎类型 | SparkSQL 实例类型   | SQL 实例类型  |
| ------------| ------------------ | ------------ |
|int32|IntegerType|int|
|int64|LongType|bigint|
|double|DoubleType|double|
|decimal|DecimalType|decimal|
|string|StringType|string|
|ObjectId|StringType|string|
|boolean|BooleanType|boolean|
|date|DateType|date|
|timestamp|TimestampType|timestamp|
|binary|BinaryType|binary|
|null|NullType|null|
|BSON(嵌套对象)|StructType|struct\<field:type,…\>|
|array|ArrayType|array\<type\>|

##SequoiaDB 存储引擎向 SparkSQL 实例类型转换的兼容性##

Y 表示兼容，N 表示不兼容

|ByteType | ShortType | IntegerType | LongType | FloatType | DoubleType | DecimalType | BooleanType|
| -----| ---- | ----- | ----- | ----- | ----- | ----- | ----- |
|int32|Y|Y|Y|Y|N|N|Y|N|
|int64|Y|Y|Y|Y|N|N|Y|N|
|double|Y|N|N|Y|N|N|Y|N|
|decimal|Y|Y|Y|Y|N|N|Y|N|
|string|Y|Y|Y|Y|N|N|Y|N|
|ObjectId|Y|N|N|Y|N|N|Y|N|
|boolean|Y|N|N|Y|N|N|Y|N|
|date|Y|Y|Y|Y|N|N|Y|N|
|timestamp|Y|Y|Y|Y|N|N|Y|N|
|binary|Y|N|N|Y|N|N|Y|N|
|null|Y|Y|Y|Y|Y|Y|Y|Y|
|BSON|Y|N|N|N|N|Y|Y|Y|
|array|Y|N|N|N|Y|N|Y|N|

>**Note:**
>
>- 不支持 SparkSQL 的 CalendarIntervalType 类型；
>- null 转换为任意类型仍为 null；
>- 不兼容类型转换时变为目标类型的零值；
>- date 和 timestamp 与数值类型转换时取其毫秒值；
>- string 如果是数值的字符串类型，则可转为对应的数值时，否则转换为 null；
>- boolean 值转为数值类型时，true 为 1，false 为 0；
>- 数值类型之间转换可能会溢出或损失精度。