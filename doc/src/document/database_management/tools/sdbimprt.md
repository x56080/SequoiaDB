sdbimprt 是 SequoiaDB 的数据导入工具，它可以将 JSON 格式或 CSV 格式的数据导入到 SequoiaDB 数据库中。

##JSON##

JSON格式的记录必须符合JSON的定义，以左右花括号作为记录的分界符，并且字符串类型的数据必须包含在两个双引号之间，转义字符为反斜杠“\\”。

##CSV##

CSV（Comma Separated Value）格式以逗号分隔数值。默认情况下记录以换行符分隔，字段以逗号分隔。用户能够指定字符串分隔符、字段分隔符以及记录分隔符。

##分隔符##

| 类型         | 默认值          |
| ------------ | --------------- |
| 字符串分隔符 | "（双引号）     |
| 字段分隔符   | ,（逗号）       |
| 记录分隔符   | '\\n'（换行符） |

>   **Note:**
>
>   *   分隔符可以使用 ASCII 码中的不可见字符，通过转义字符“\\”输入 ASCII 码的十进制数值（0 ~ 127），例如“\\30”。回车符、换行符、制表符、转义字符“\\”可以直接使用“\\r”，“\\n”，“\\t”，“\\\\”。
>   *   可以使用 UTF-8 字符作为分隔符。
>   *   可以使用多个字符作为分隔符。

##CSV类型##

| 类型          | 别名    | 说明 |
| ------------- | ------- | ---- |
| int           | integer | 十进制整型，取值范围为 -2147483648 ~ 2147483647 |
| long          |         | 十进制长整型，取值范围 -9223372036854775808 ~ 9223372036854775807 |
| double        |         | 双精度浮点型，取值范围为 1.79E +/- 308 (15 位) |
| decimal       |         | 高精度数，范围为小数点前最高 131072 位;小数点后最高 16383 位。<br>可以指定精度，如 ```decimal(18, 6)``` |
| number        |         | 数值类型，自动判断数值的具体类型（int，long，double，decimal） |
| bool          | boolean | 布尔型，取值可以为 true，false，t，f，yes，no，y 或 n，不区分大小写 |
| string        |         | 字符串 |
| null          |         | 空值 |
| oid           |         | OID 类型，长度必须为 24 个字符，不支持类型自动判断 |
| date          |         | 日期类型，取值范围为 0000-01-01 ~ 9999-12-31，不支持类型自动判断 |
| autodate      |         | 日期类型，取值范围为 0000-01-01 ~ 9999-12-31，不支持类型自动判断 |
| timestamp     |         | 时间戳类型，取值范围为 1902-01-01-00.00.00.000000 ~ 2037-12-31-23.59.59.999999，不支持类型自动判断。<br> 可以指定格式，如``timestamp("YYYY-MM-DD HH:mm:ss")`` | 
| autotimestamp |         | 时间戳类型，取值范围为 1902-01-01-00.00.00.000000 ~ 2037-12-31-23.59.59.999999，不支持类型自动判断 | 
| binary        |         | 二进制类型，使用 base64 编码，不支持类型自动判断 |
| regex         |         | 正则表达式类型，不支持类型自动判断 |
| skip          |         | 忽略指定的列，该列的数据不导入到数据库 |

>   **Note:**
>
>   *   int、long、double 支持以‘#’开头的数字，例如“#123.456”。
>   *   double 支持科学计数法，例如“1.23e-4”，“-1.23E+4”。
>   *   double 支持忽略小数点前的“0”，例如“.123”。
>   *   在自动判断类型时，整数超过 long 的范围，浮点数超过 double 的范围，以及浮点数总位数超过 15 位或小数位超过 6 位时，类型判断为 decimal。
>   *   autodate 类型支持使用整数，表示自 1970-01-01-00.00.00.000000 以来的毫秒数, 取值范围为 long 类型的范围。
>   *   autotimestamp 类型支持使用整数，表示自 1970-01-01-00.00.00.000000 以来的毫秒数，取值范围为 -2147414400000 ~ 2147443199000。

##CSV类型自动判断##

在不指定 CSV 字段类型时，导入工具会自动判断类型。其中 oid、date、timestamp、binary、regex 不支持自动类型判断，会被识别为 string 类型。整数超过 long 的范围，浮点数超过 double 的范围，以及浮点数总位数超过 15 位或小数位超过 6 位时，类型判断为 decimal。

例如：

| CSV 数据            | 判断类型 | 实际数据            |
| ------------------- | -------- | ------------------- |
| 123                 | int      | 123                 |
| 123.                | int      | 123                 |
| +123                | int      | 123                 |
| -123                | int      | -123                |
| 0123                | int      | 123                 |
| #-123.              | int      | -123                |
| 2147483648          | long     | 2147483648          |
| 123.1               | double   | 123.1               |
| .123                | double   | 0.123               |
| 9223372036854775808 | decimal  | 9223372036854775808 |
| true                | bool     | true                |
| false               | bool     | false               |
| "123"               | string   | "123"               |
| 123a                | string   | "123a"              |
| "true"              | string   | "true"              |
| "false"             | string   | "false"             |
| "null"              | string   | "null"              |
| null                | null     | null                |

##CSV类型转换##

在指定 CSV 字段类型时，导入工具会将字段转换为指定的类型。如果字段的实际类型不是指定的类型，则转换可能失败。具体参考下表，最左边一列是指定的类型，Y 表示可以转换，N 表示不能转换。

| 指定类型 \\ 实际类型 | int | long | double | decimal | bool | string | null | oid | date | timestamp | binary | regex |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| int | Y | 可能溢出 | 可能丢失精度 | 可能丢失精度 | Y | 支持数值字符串 | Y | N | N | N | N | N |
| long | Y | Y | 可能丢失精度 | 可能丢失精度 | Y | 支持数值字符串 | Y | N | N | N | N | N |
| double | Y | Y | Y | 可能丢失精度 | N | 支持数值字符串 | Y | N | N | N | N | N |
| decimal | Y | Y | Y | Y | N | 支持数值字符串 | Y | N | N | N | N | N |
| number | Y | Y | Y | Y | Y | 支持数值字符串 | Y | N | N | N | N | N |
| bool | Y | Y | N | N | Y | 支持 bool 字符串 | Y | N | N | N | N | N |
| string | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| null | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| oid | N | N | N | N | N | 支持 OID 字符串 | N | Y | N | N | N | N |
| date | Y | Y | N | N | N | 支持 date 字符串 | N | N | Y | Y | N | N |
| timestamp | Y | Y | N | N | N | 支持 timestamp 字符串 | N | N | Y | Y | N | N |
| binary | N | N | N | N | N | 支持 binary 字符串 | N | N | N | N | Y | N |
| regex | N | N | N | N | N | 支持 regex 字符串 | N | N | N | N | N | Y |

>   **Note:**
>
>   *   指定类型为 bool，实际类型为 int、long 时，0 值转为 false，非 0 值转为 true。
>   *   指定类型为 int、long，实际类型为 bool 时，true、t、yes 或 y 转为 1，false、f、no 或 n 转为 0。
>   *   参数 --cast 可以指定数值转换时是否允许精度损失或数值溢出。

##命令选项##

###通用选项###

| 选项        | 缩写 | 说明 |
| ----------- | ---- | ---- |
| --help      | -h   | 显示帮助信息 |
| --version   | -V   | 显示版本号 |
| --hosts     |      | 指定主机地址（hostname:svcname），用“,”分隔多个地址，默认值为“localhost:11810” |
| --user      | -u   | 数据库用户名 |
| --password  | -w   | 数据库用户名对应的密码，指定值则使用明文输入，不指定值则命令行提示输入        |
| --cipher    |      | 是否使用加密文件输入密码，默认值为 false |
| --token     |      | 加密口令 |
| --cipherfile|      | 加密文件，默认值为 ./passwd |
| --csname    | -c   | 集合空间的名字 |
| --clname    | -l   | 集合的名字 |
| --errorstop |      | 如果遇到解析错误就停止，默认值为 false |
| --ssl       |      | 使用 SSL 连接，默认值为 false |
| --verbose   | -v   | 显示详细的执行信息 |

###输入选项###

| 选项           | 缩写 | 说明 |
| -------------- | ---- | ---- |
| --file         |      | 要导入的数据文件的名称，使用“,”分隔多个文件或目录。<br>如果 --file 和 --exec 都没有指定，则从标准输入读取数据 |
| --exec         |      | 执行外部程序来获取数据，外部程序必须将数据输出到标准输出 |
| --type         |      | 导入数据格式，可以是 csv 或 json，默认为 csv |
| --linepriority |      | 指定分隔符的优先级，默认 auto。<br>auto：当 type 为 csv 时，记录分隔符最高优先级；当 type 为 json 时，字符串分隔符最高优先级。<br>true：记录分隔符 > 字符串分隔符 > 字段分隔符。<br>false：字符串分隔符 > 记录分隔符 > 字段分隔符。 |
| --delrecord    | -r   | 指定记录分隔符，默认值为换行符'\\n' |
| --force        |      | 如果数据中有非 UTF-8 的字符，强制导入数据，默认为 false |

>   **Note:**
>
>   *   --linepriority 参数需要被特别关注，如果设置不当，可能会导入数据失败。当记录中包含记录分隔符，并且 --linepriority 为 true 时，工具会优先按照记录分隔符解析，而导致导入失败。例如：如果记录为 ```{"name": "Mike\n"}```，应当设置 --linepriority 为 false。
>   *   使用 --file 参数指定文件或目录时，重复出现的文件会被忽略。
>   *   使用 --hosts 指定地址时，重复出现的地址会被忽略。

###JSON选项###
| 选项           | 缩写 | 说明 | 
| -------------- | ---- | ---- | 
| --unicode      |      | 是否转义unicode字符编码（\uXXXX），默认为 true | 

###CSV选项###

| 选项           | 缩写 | 说明 | 
| -------------- | ---- | ---- | 
| --delchar      | -a   | 指定字符串分隔符，默认值为双引号'"' | 
| --delfield     | -e   | 指定字段分隔符,默认值为逗号',' | 
| --fields       |      | 指定导入数据的字段名、类型、默认值 | 
| --datefmt      |      | 指定日期格式，默认为 YYYY-MM-DD | 
| --timestampfmt |      | 指定时间戳格式，默认为 YYYY-MM-DD-HH.mm.ss.ffffff | 
| --trim         |      | 删除字符串左右两侧的空格（包括 ASCII 空格和 UTF-8 全角空格），<br>取值可以是 no、right、left 或 both， 默认值为 no | 
| --headerline   |      | 指定导入数据首行是否作为字段名，默认值为 false | 
| --sparse       |      | 指定导入数据时是否自动添加字段名，默认值为 true，字段名按 filed1、field2 顺序增加 | 
| --extra        |      | 指定导入数据时是否自动添加值，默认值为 false | 
| --cast         |      | 指定是否允许数值类型转换时丢失精度或数值溢出，默认值为 false | 
| --strictfieldnum|     | 指定是否严格限制记录的字段数与定义的字段数一致，默认值为 false |
| --checkdelimeter|     | 是否严格校验分隔符，默认为 true。<br>true：禁止字符分隔符、字段分隔符、记录分隔符互相包含；<br>false：允许字符分隔符、字段分隔符、记录分隔符互相包含。|

>   **Note:**
>
>   *   fields 语法：```fieldName [type [default <value>], ...]```
>       *   type 支持所有的 CSV 类型
>       *   type 可不写，由导入工具自动判断
>       *   指定字段可以用命令行指定，也可以在导入文件的首行指定。如果在命令行指定了 --fields，并且 --headerline 设为 true，导入工具将会优先使用命令行指定字段并且跳过导入文件的首行
>       *   字段名不能以“$”开头，中间不能有“.”，不能有不可见字符，包含空格时需要将字段名包含在单引号或双引号中
>       *   decimal 类型可以指定精度，如 ```decimal(18, 6)```
>       *   例如：```--fields='name string default "Jack", age int default 18, phone'```
>   *   datefmt 格式包括年、月、日、通配符以及特定字符
>       *   年：YYYY
>       *   月：MM
>       *   日：DD
>       *   通配符：*
>       *   特定字符：任意 UTF-8 字符
>       *   其中年、月、日必须是整数，并且符合日期类型的范围
>       *   指定通配符时，日期字段上对应的位置可以为任意字符
>       *   指定特定字符时，日期字段上对应的位置必须为该指定字符
>       *   例如需要导入的数据中日期格式为“3/15, 2015”，则设置 ```--datefmt="MM/DD, YYYY"``` 与该格式匹配
>   *   timestamp 格式包括年、月、日、时、分、秒、微秒或毫秒、通配符以及特定字符
>       *   年：YYYY
>       *   月：MM
>       *   日：DD
>       *   时：HH
>       *   分：mm
>       *   秒：ss
>       *   微秒：ffffff
>       *   毫秒：SSS
>       *   时区：Z
>       *   通配符：*
>       *   特定字符：任意 UTF-8 字符
>       *   其中年、月、日、时、分、秒、微秒、毫秒必须是整数，并且符合时间戳类型的范围
>       *   微秒和毫秒不能同时出现，只能出现其中一个
>       *   指定通配符时，时间戳字段上对应的位置可以为任意字符
>       *   指定特定字符时，时间戳字段上对应的位置必须为该指定字符
>       *   例如需要导入的数据中时间戳格式为“3/15/2015 T 12.30.123”，则设置 ```--timestampfmt="MM/DD/YYYY T mm.ss.SSS"``` 与该格式匹配
>       *   例如指定带时区的时间戳：``--timestampfmt="YYYY-MM-DD HH.mm.ssZ"``
>       *   例如指定东八区时间戳： ``--timestampfmt="YYYY-MM-DD HH.mm.ss+0800"``

###导入选项###

| 选项          | 缩写 | 说明 |
| ------------- | ---- | ---- |
| --insertnum   | -n   | 指定每次导入的记录数，取值范围为 1 ~ 100000，默认值为 100。 |
| --jobs        | -j   | 指定导入连接数（每个连接一个线程），取值范围为 1 ~ 1000，默认值为 1。 |
| --coord       |      | 指定是否自动查找协调节点，默认值为 true。 |
| --sharding    |      | 指定是否按分区信息重新打包记录，默认值为 true。 |
| --transaction |      | 指定导入数据时是否开启事务，默认为 false。<br>**注意：此功能需要服务端开启事务。** |
| --allowkeydup |      | 指定是否允许唯一索引的键出现重复时忽略错误继续导入，默认值为 true。 |

##示例##

1.  将数据导入到本地数据库 11810 中集合空间 foo 的集合 bar，导入格式是 csv，数据文件为“test.csv”，第一行为字段定义：

    以下是导入文件的内容：

    ```lang-csv
    name string default "Anonymous", age int, country
    "Jack",18,"China"
    "Mike",20,"USA"
    ```

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c foo -l bar --headerline=true
    ```

2.  导入格式是 csv，文件名是“test.csv”，导入至集合空间 foo 的集合 bar 中

    以下是导入文件的内容：

    ```lang-csv
    "Jack",18,"China"
    "Mike",20,"USA"
    ```

    导入命令：

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c foo -l bar --fields='name string default "Anonymous", age int, country'
    ```

3.  导入格式是 csv，文件名是“test.csv”，导入至集合空间 foo 的集合 bar 中

    以下是导入文件的内容，其中文件第一行是字段定义，需要跳过：

    ```lang-csv
    name, age, country
    "Jack",18,"China"
    "Mike",20,"USA"
    ```

    导入命令：

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c foo -l bar --fields='name string default "Anonymous", age int, country' --headerline=true
    ```

4.  导入格式是 csv，导入文件是目录“../data”中的所有文件，导入至集合空间 foo 的集合 bar 中

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=../data -c foo -l bar --fields='name string default "Anonymous", age int, country'
    ```

5.  导入格式是 csv，导入文件是目录“../data”中的所有文件以及“./foo_bar_data.csv”，导入至集合空间 foo 的集合 bar 中，有 11810 和 11910 两个协调节点，记录中时间戳类型的数据类似于“2015-10-01 T 12.31.15.123 T”，使用两个连接同时导入

    ```lang-bash
    $ sdbimprt --type=csv --file=../data,./foo_bar_data.csv --fields='name, time timestamp' -c foo -l bar --timestampfmt="YYYY-MM-DD T HH.mm.ss.SSS T" --hosts=localhost:11810,localhost:11910 -j 2
    ```

6.  导入格式是 json，通过管道从其它工具 other 获取数据，导入至集合空间 foo 的集合 bar 中

    ```lang-bash
    $ other | sdbimprt --hosts=localhost:11810 --type=json -c foo -l bar 
    ```
7.   导入格式是 csv，导入多种时间戳格式

    例子的系统时区是东八区

    以下是导入文件的内容：

    ```lang-csv
    2014-01-01, 2001/01/01, 1990-01-01
    2014-01-01Z, 2001/01/01Z, 1990-01-01Z
    2014-01-01+0200, 2001/01/01+0200, 1990-01-01+0200
    ```

    导入命令：

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c foo -l bar --fields='time1 timestamp("YYYY-MM-DD"), time2 timestamp("YYYY/MM/DDZ"), time3 timestamp("YYYY-MM-DD+0600")'
    ```

    1. time1没有指定时区，因此都用系统的时区
    2. time2指定时区字符Z，如果数据没有时区信息，则用系统的时区；如果数据有Z字符，则作为UTC时间
    3. time3指定+0600时区，如果数据没有时区信息，则用字段指定的+0600时区；如果数据有Z字符，则作为UTC时间

    查询结果：

    ```lang-javascript
    > db.foo.bar.find()
    {
       "_id": {
          "$oid": "5ad5565f13f513e620000000"
       },
       "time1": {
          "$timestamp": "2014-01-01-00.00.00.000000"
       },
       "time2": {
          "$timestamp": "2001-01-01-00.00.00.000000"
       },
       "time3": {
          "$timestamp": "1990-01-01-02.00.00.000000"
       }
    }
    {
       "_id": {
          "$oid": "5ad5565f13f513e620000001"
       },
       "time1": {
          "$timestamp": "2014-01-01-00.00.00.000000"
       },
       "time2": {
          "$timestamp": "2001-01-01-08.00.00.000000"
       },
       "time3": {
          "$timestamp": "1990-01-01-08.00.00.000000"
       }
    }
    {
       "_id": {
          "$oid": "5ad5565f13f513e620000002"
       },
       "time1": {
          "$timestamp": "2014-01-01-00.00.00.000000"
       },
       "time2": {
          "$timestamp": "2001-01-01-06.00.00.000000"
       },
       "time3": {
          "$timestamp": "1990-01-01-06.00.00.000000"
       }
    }
    ```