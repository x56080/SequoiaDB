sdbexprt 是一个实用的工具。它可以将集合从 SequoiaDB 数据库导出到 JSON 格式或者 CSV 格式的数据存储文件。sdbexprt 支持将一个集合导出到一个文件中，同时也支持将多个集合批量导出到指定目录下。

##JSON##

JSON 导出格式中的 JSON 记录符合 JSON 的定义，以左右花括号作为 JSON 记录的分界符，并且字符串类型的数据必须包含在两个双引号之间，转义字符为反斜杠“\\”。默认情况下（SequoiaDB的）记录以换行符分隔。用户能够指定记录分隔符。

##CSV##

CSV（Comma Separated Value）导出格式以逗号分隔数值。默认情况下记录以换行符分隔，字段以逗号分隔。用户能够指定字符串分隔符、字段分隔符以及记录分隔符。

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

##选项##

###通用选项###

| 选项        | 缩写 | 说明 |
| ----------- | ---- | ---- |
| --help      | -h   | 显示帮助信息 |
| --version   |      | 显示版本信息 |
| --hosts     |      | 指定主机地址（hostname:svcname），用“,”分隔多个地址，默认值为 localhost:11810 |
| --user      | -u   | 数据库用户名 |
| --password  | -w   | 数据库密码，指定值则使用明文输入，不指定值则命令行提示输入 |
| --cipher    |      | 是否使用加密文件输入密码，默认为 false |
| --token     |      | 加密口令 |
| --cipherfile|      | 加密文件，默认为 ./passwd |
| --delrecord | -r   | 记录分隔符。默认是'\\n' |
| --type      |      | 导出数据格式，为 csv 或 json，默认为 csv |
| --filelimit |      | 指定单个导出文件的大小上限，单位可以为 k、K、M、m、G、g、T 或 t，默认值为 16G。<br>当导出文件将超过限制时，会切分为多个文件，具有编号后缀，如 file.csv，file.csv.1，file.csv.2 |
| --fields    |      | 导出集合的字段。该选项可以指定多次以指定多个导出集合的字段。<br>格式为 ```[csName.clName:][field1[,...]]``` ，当确定只导出一个集合时，可以仅指定字段列表 ```[field1[,...]]``` |
| --withid    |      | 强制导出或者在配置文件中生成字段时，是否包含 _id 字段，默认为false |
| --floatfmt  |      | 指定浮点数格式，默认是 '%.16g'，输入 'db2' 表示 '%+.14E'，其他格式 %[+][.precision]\(f\|e\|E\|g\|G\) |
| --ssl       |      | 使用 SSL 连接，默认 false |
| --replace   |      | 覆盖导出数据文件 |

###单集合选项###

| 选项     | 缩写 | 说明 |
| -------- | ---- | ---- |
| --csname | -c   | 导出数据的集合空间名 |
| --clname | -l   | 导出数据的集合名 |
| --file   |      | 导出的文件名 |
| --select |      | 选择规则，例如：```--select '{ age:"", address:{$trim:1} }'```<br>不能和选项 --fields 同时使用 |
| --filter |      | 导出过滤条件，例如：--filter '{ age: 18 }' |
| --sort   |      | 导出数据排序条件，例如：```--sort '{ name: 1 }'``` |
| --skip   |      | 指定从第几条记录开始导出，默认是 0 |
| --limit  |      | 指定导出的记录数，默认是 -1 （导出所有记录） |

###多集合选项###

| 选项          | 缩写 | 说明 |
| ------------- | ---- | ---- |
| --cscl        |      | 导出的若干个导出集合或集合空间，多个名称使用逗号分隔，如 ```--cscl cs1,cs2.cla``` |
| --excludecscl |      | 不包含的集合或集合空间，类似 --cscl |
| --dir         |      | 导出的目录。导出的每一个集合对应目录中的同名文件，如 foo.bar.csv |

###JSON 选项###

| 选项            | 缩写 | 说明 |
| --------------- | ---- | ---- |
| --strict        |      | 是否严格按照数据类型导出，默认值为 false |

###CSV 选项###

| 选项            | 缩写 | 说明 |
| --------------- | ---- | ---- |
| --delchar       | -a   | 字符分隔符，默认值为双引号'"' |
| --delfield      | -e   | 字段分隔符，默认值为逗号',' |
| --included      |      | 是否导出字段名到文件首行，默认值为 true |
| --includebinary |      | 是否导出完整二进制数据，默认值为 false |
| --includeregex  |      | 是否导出完整的正则表达式，默认值为 false |
| --force         |      | 对于导出 csv 格式，每个集合必须指定对应的字段，否则不允许导出；<br>--force 选项可以强制导出，未指定字段的集合默认为第一行记录中除了 _id 以外的字段 |
| --kicknull      |      | 是否踢掉 null 值，true 输出空字符，false 输出 null，默认为 false | 
| --checkdelimeter|      | 是否严格校验分隔符，默认为 true。<br>true：禁止字符分隔符、字段分隔符、记录分隔符互相包含；<br>false：允许字符分隔符、字段分隔符、记录分隔符互相包含。|

###配置文件选项###

| 选项        | 缩写 | 说明 |
| ----------- | ---- | ---- |
| --genconf   |      | 指定一个配置文件名，将当前命令行中所指定的选项和值按照“键=值”的方式写入到配置文件，不执行导出工作 |
| --genfields |      | 生成配置文件时，是否对每一个集合生成对应的 --fields 选项，默认值为 true |
| --conf      |      | 指定一个配置文件作为输入，如果命令中和配置文件中存在相同的选项，优先选择命令行中的值 |

> **Note:**
>
>  * 导出工具支持单集合导出和多集合批量导出， **单集合选项** 只能用于导出一个集合，但具有更灵活的导出条件选项，如过滤、排序。
>  * 导出多集合到 csv 格式时，必须使用 --fields 选项对每一个集合指定字段，工具提供的 --genconf 选项将每一个集合的第一行记录的字段导出到配置文件中的 --fields 选项，可以比较方便地编辑每一个集合的字段。
>  * --genconf 选项将当前命令行的选项写入到配置文件中，下次使用 --conf 选项指定配置文件执行即可，这提供一种多次执行相似命令的便捷方式，另外这种方式主要用于在多集合导出 csv 情况下，对每一个集合生成对应的 --fields 选项。
>  * 导出单集合时，--select 具有和 --fields 选项一样的作用，但 --select 选项更加灵活。
>  * 当不指定导出任何集合或者集合空间，即 -c、-l、--cscl 都不指定，则导出数据库中所有的集合。
>  * 当使用配置文件的选项和命令行选项一样时，优先选择命令行值。

##返回值##

* 0:   执行成功
* 非0: 执行失败

##示例##

1.  导出集合“foo.bar”，导出格式为 csv，导出文件为“foo.bar.csv”，指定字段“field1”、“fieldNotExist”、“field3”，其中字段“fieldNotExist”在集合中不存在

    ```lang-bash
    $ sdbexprt -s localhost -p 11810 --type csv --file foo.bar.csv --fields field1,fieldNotExist,field3 -c foo -l bar
    ```

    导出的foo.bar.csv的内容可能如下：

    ```
    field1, fieldNotExist, field3
    "Jack",,"China"
    "Mike",,"USA"
    ```

2.  导出数据库中所有的集合，排除集合空间“cs1”和集合“cs2.cla”以外，导出文件到目录“exportpath”下

    ```lang-bash
    $ sdbexprt --type json --dir exportpath --excludecscl cs1,cs2.cla
    ```

3.  导出一个集合空间中所有的集合和另外一个集合，排除一个集合，导出 csv 格式，由于必须指定每一个集合的 --fields，使用 --force 选项强制导出

    ```lang-bash
    $ sdbexprt --dir exportpath --cscl cs1.cla,cs2 --excludecscl cs2.cla --force true
    ```

4.  同上例，生成配置文件模板，其中配置文件中包含每一个所对应的 --fields 选项；根据需求修改配置文件之后，再执行导出

    生成配置文件：

    ```lang-bash
    $ sdbexprt --dir exportpath --cscl cs1.cla,cs2 --excludecscl cs2.cla --genconf export.conf
    ```

    配置文件文件内容可能如下：

    ```lang-ini
    hostname = localhost
    ...
    dir = exportpath/
    cscl = cs1.cla,cs2
    excludecscl = cs2.cla
    fields = cs1.cla: a1, a2, a3
    fields = cs2.clb: b1, b2, b3
    fields = cs2.clc: c1, c2
    fields = cs2.cld: d1, d2, d3, d4
    ```

    执行导出：

    ```lang-bash
    $ sdbexprt --conf export.conf
    ```
