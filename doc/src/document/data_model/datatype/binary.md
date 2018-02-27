##概念##

在 SequoiaDB 中的数据使用 JSON 形式访问，因此对于二进制的数据需要用户使用 Base64 方式进行编码，之后以字符串的形式发送至数据库。

##格式##

二进制数据的表达形式如下：

```
{ "$binary" : "<数据>", "$type" : <类型> }
```

其中“数据”必须为 Base64 编码的数据，“类型”为 0-255 之间的十进制数值，用户可以任意指定该范围之间的数值作为应用程序中的类型标示。

Base64 为一种通用的数据转换形式，主要将二进制数据转化为以纯 ASCII 字符串表示的字节流。一般来说转换之后的数据长度会大于原本的数据长度。

为了节省空间，在 SequoiaDB 的内部存放数据时，会将 Base64 编码后的数据解码为原始数据进行存放。当用户读取数据时会再次将其转化为 Base64 形式发送。

>  **Note:**
>
>  请参考 [BinData](reference/Sequoiadb_command/SpecialObjects/BinData.md)。

##示例##

字符串“hello world”被 Base64 编码后的数据为“aGVsbG8gd29ybGQ=”。包含“helloworld”二进制数据，且类型为 1 的 JSON 数据为：

```
{ "key" : { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } }
```
