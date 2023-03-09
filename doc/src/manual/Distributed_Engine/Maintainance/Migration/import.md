[^_^]:
    数据导入

SequoiaDB 巨杉数据库支持使用 [sdbimprt 工具][sdbimprt]，快速导入 CSV 或 JSON 格式的数据。用户可通过该工具将以下数据导入 SequoiaDB 集群中：

- 从其他数据库导出的数据
- [sdbexprt 工具][sdbexprt]导出的数据
- 用户程序生成的数据

下述将根据数据的格式分别介绍数据导入的具体操作步骤。示例中均以 `localhost:11810` 作为导入的目标集群协调节点地址，sample.employee 作为导入的目标集合，“sdbadmin”作为数据库用户名，及 `~/sequoiadb/passwd` 作为用户密码对应的密文文件路径。

##导入 CSV 数据##

CSV（Comma-Separated Values）是数据库间通用的数据交换格式之一，其文件以纯文本的形式存储表格数据。默认情况下记录间以换行符（\\n）分隔，字段间以逗号（,）分隔。CSV 格式定义可以参考 [RFC 4180][rfc4180] 文档。 

> **Note**：
>
> 在导入数据前，用户需要确保 CSV 文件的编码格式为 UTF-8。

###单一导入###

将文件 `test.csv` 中的数据通过协调节点导入至集合 sample.employee 中

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type csv --file test.csv -c sample -l employee --headerline true
```

###批量导入###

批量导入可分为导入整个数据目录和导入多个目录的数据文件。

**导入数据目录**

1. 待导入的数据目录 `/opt/data` 中，所有 CSV 格式的数据文件均已存在字段定义

    ```lang-text
    id,name,age,identity,phone_number,email,country
    1,"Jack",18,"student","18921222226","jack@example.com","China"
    2,"Mike",20,"student","18923244255","mike@example.com","USA"
    3,"Woody",25,"worker","18945253245","woody@example.com","China"
    ```

2. 将目录 `/opt/data` 中的数据导入集合中，并通过参数 --headerline 指定文件首行作为字段定义

    ```lang-bash
    $ sdbimprt --hosts "localhost:11810" --type csv -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file /opt/data --headerline true
    ```

**导入多个数据文件**

1. 待导入的数据文件包括 `/opt/data1.csv`、`/opt/data2.csv` 和 `/opt/test/data3.csv`，各文件包含的内容如下：

    ```lang-text
    data1.csv 存在如下数据：
    1,"Jack",18
    2,"Mike",20
    3,"Woody",25
    ···

    data2.csv 存在如下数据：
    101,"Alice",28
    102,"Mary",37
    103,"Helen",19
    ···

    data3.csv 存在如下数据：
    201,"Nancy",83
    202,"Mary",62
    203,"Eva",22
    ···
    ```

2. 将数据导入集合中，并通过参数 --fields 指定字段定义

    ```lang-bash
    $ sdbimprt --hosts "localhost:11810" --type csv -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file /opt/data1.csv,/opt/data2.csv,/opt/test/data3.csv --fields 'id long,name string default "Anonymous",age int'
    ```

    > **Note：**
    >
    > - 参数 --fields 语法为 fieldName [type [default <value>], ...]。不填入 type 时，系统将自动判断字段的数据类型。
    > - 参数 --file 支持指定多个文件或者目录，使用逗号（,）分隔，重复出现的文件会被忽略。

###导入大数据文件###

在导入文件的数据量较大时，用户可以增加每次导入的记录数（对应参数 -n）以及导入连接数（对应参数 -j），以提升导入效率。

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type csv -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file /opt/data --headerline true -n 5000 -j 6
```

###自定义记录分隔符导入###

CSV 格式中，默认以逗号（,）作为字段分隔符。当用户数据文件的分隔符与默认值不符时，可以在导入时通过参数 --delfield 自定义分隔符，避免导入操作报错。 

待导入的数据文件 `/opt/data4.csv` 中存在以下数据，其中字段分隔符为"|"：

```lang-text
1|"Jack"|18|"student"|"18921222226"|"jack@example.com"|"China"
2|"Mike"|20|"student"|"18923244255"|"mike@example.com"|"USA"
3|"Woody"|25|"worker"|"18945253245"|"woody@example.com"|"China"
```

将数据文件 `data.csv` 中的数据导入集合中，并通过参数 --delfield 自定义字段分隔符为"|"

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type csv -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --fields 'id long,name string default "Anonymous",age int,identity,phone_number,email,country' --delfield "|" --file /opt/data4.csv
```

##导入 JSON 数据##

JSON（JavaScript Object Notation）是一种轻量级的数据交换格式。在 JSON 文件中，记录间需以左右花括号分隔，字符串类型的数据需包含在两个双引号（""）之间。当字符串类型的数据包含特殊字符时，需要使用反斜杠（\\）进行转义。

###单一导入###

将文件 `test.json` 中的数据通过协调节点导入至集合 sample.employee 中

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type json --file test.json -c sample -l employee
```

###批量导入###

批量导入可分为导入整个数据目录和导入多个不同目录的数据文件。

**导入数据目录**

将数据目录 `/opt/data` 中的数据导入集合中

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type json -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --dir /opt/data
```

**导入多个数据文件**

将数据文件 `/opt/data1.json`、`/opt/data2.json` 和 `/opt/test/data3.json` 导入集合中

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type json -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file /opt/data1.json,/opt/data2.json,/opt/test/data3.json
```

> **Note：**
>
> 参数 --file 支持指定多个文件或者目录，使用逗号（,）分隔，重复出现的文件会被忽略。


###导入大数据文件###

在导入文件的数据量较大时，用户可以增加每次导入的记录数（对应参数 -n）和导入连接数（对应参数 -j），以提升导入效率。

```lang-bash
$ sdbimprt --hosts "localhost:11810" --type json -c sample -l employee --user sdbadmin --cipher true --cipherfile ~/sequoiadb/passwd --file /opt/data -n 5000 -j 6
```





[^_^]:
    本文使用的所有引用及链接
[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[csv_data_type]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md#csv_data_type
[rfc4180]:https://www.rfc-editor.org/rfc/rfc4180.txt