SequoiaDB 巨杉数据库除了基本类型对象，还包含特殊类型对象。

特殊类型对象如下：

| 名称 | 描述 |
|------|------|
| BinData | Base64 形式的二进制数据 |
| BSONArray | BSON 数组 |
| BSONObj | BSON 对象 |
| MaxKey | 所有数据类型中的最大值 |
| MinKey | 所有数据类型中的最小值 |
| NumberDecimal | 高精度数，可以保证精度不丢失 |
| NumberLong | 长整型 |
| OID | 对象 ID 为一个 12 字节的 BSON 数据类型 |
| Regex | 正则表达式 |
| SdbDate | YYYY-MM-DD 形式的日期 |
| Timestamp | YYYY-MM-DD-HH.mm.ss.ffffff 形式的时间戳 |