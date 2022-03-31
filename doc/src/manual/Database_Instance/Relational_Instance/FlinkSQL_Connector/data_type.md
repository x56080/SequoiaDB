[^_^]:
    FlinkSQL 连接器-数据类型映射

本文档主要介绍 SequoiaDB 巨杉数据库与 FlinkSQL 连接器的数据类型映射，以及两者间数据类型转换的兼容性。

##数据类型映射表##

| SequoiaDB 数据类型  |   FlinkSQL 数据类型    |
| ---------------     | --------------------   |
|   int32             |          INT           |
|   int64             |         BIGINT         |
|   double            |         DOUBLE         |
|   decimal           |        DECIMAL         |
|   string            |  CHAR/VARCHAR/STRING   |
|   OID               |         STRING         |
|   boolean           |        BOOLEAN         |
|   date              |          DATE          |
|   timestamp         |       TIMESTAMP        |
|   binary            | BINARY/VARBINARY/BYTES |
|   null              |         不支持         |

##数据类型兼容表##
|SDB\ROWDATA|TINYINT|SMALLINT|INT|BIGINT|FLOAT|DOUBLE|DATE|TIMESTAMP|BOOLEAN|DECIMAL|CHAR/VARCHAR/STRING|BINARY/VARBINARY/BYTES|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|**int32**|可能溢出|可能溢出|Y|Y|Y|Y|可能超出DATE范围|可能超出TIMESTAMP范围|Y|Y|Y|Y|
|**int64**|可能溢出|可能溢出|可能溢出|Y|Y|Y|可能超出DATE范围|可能超出TIMESTAMP范围|Y|Y|Y|Y|
|**double**|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|Y|N|N|Y|Y|Y|Y|
|**decimal**|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|N|N|Y|Y|Y|Y|
|**string**|支持数值字符串|支持数值字符串|支持数值字符串|支持数值字符串|支持数值字符串|支持数值字符串|支持Date字符串|支持Timestamp字符串|支持Bool字符串|支持数值字符串|Y|Y|
|**OID**|N|N|N|N|N|N|N|N|N|N|Y|Y|
|**boolean**|Y|Y|Y|Y|Y|Y|N|N|Y|Y|Y|Y|
|**date**|可能溢出|可能溢出|可能溢出|Y|可能精度丢失|可能精度丢失|Y|Y|N|Y|Y|Y|
|**timestamp**|可能溢出|可能溢出|可能溢出|Y|可能精度丢失|可能精度丢失|Y|Y|N|Y|Y|Y|
|**binary**|N|N|N|N|N|N|N|N|N|N|Y|Y|

>**Note:**
>
> - 不兼容的数据类型发生转换时，原数据将转换为目标类型的零值。
> - string 类型编码格式为 UTF-8。
> - string 类型支持将数值字符串转换为 TINYINT、SMALLINT、INT、BIGINT、FLOAT、DOUBLE 等数值类型。如果转换发生溢出，则视为数据类型不兼容。
> - string 类型支持将"yyyy-MM-dd.HH:mm:ss"格式的日期字符串，转换为 DATE 或 TIMESTAMP 类型，格式不匹配时将转换为零值。
> - boolean 类型转换为数值类型时，true 对应 1，false 对应 0。
> - Flink SQL 中 DECIMAL(p,s) 支持最大精度为 38，数值超出精度表示范围时，视为数据类型不兼容。仅小数部分超出精度时，将舍弃小数部分。