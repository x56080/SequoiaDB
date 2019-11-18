## 数据类型映射表

MySQL 支持多种 SQL 数据类型：数值类型，date 和 time 类型，字符串类型等。

从 MySQL 实例到 SequoiaDB 的 JSON 对象实例之间的数据类型映射关系为：

| MySQL 实例 | JSON 对象实例| 备注                                         |
| ---------- | ------------ | -------------------------------------------- |
| BIT        | int/long     | 超出int范围则按long存储                      |
| BOOL       | int          |                                              |
| TINYINT    | int          |                                              |
| SMALLINT   | int          |                                              |
| MEDIUMINT  | int          |                                              |
| INT        | int/long     | 超出int范围则按long存储                      |
| BIGINT     | long/decimal | 超出long范围则按DECIMAL存储                  |
| FLOAT      | double       |                                              |
| DOUBLE     | double       |                                              |
| DECIMAL    | decimal      |                                              |
| YEAR       | int          |                                              |
| DATE       | date         |                                              |
| TIME       | double       | 'HHMMSS[.fraction]'格式的double值            |
| DATETIME   | string       | 'YYYY-MM-DD HH:MM:SS[.fraction]'格式的字符串 |
| TIMESTAMP  | timestamp    |                                              |
| CHAR       | string       |                                              |
| VARCHAR    | string       |                                              |
| BINARY     | binary       |                                              |
| VARBINARY  | binary       |                                              |
| TINYBLOB   | binary       |                                              |
| BLOB       | binary       |                                              |
| MEDIUMBLOB | binary       |                                              |
| LONGBLOB   | binary       | 最大长度16MB                                 |
| TINYTEXT   | string       |                                              |
| TEXT       | string       |                                              |
| MEDIUMTEXT | string       |                                              |
| LONGTEXT   | string       | 最大长度16MB                                 |
| ENUM       | int          |                                              |
| SET        | int          |                                              |
| JSON       | binary       |                                              |
| GEOMETRY   | 不支持       |                                              |
| NULL       | -            | 不存储                                       |
