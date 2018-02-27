SequoiaDB 数据库使用 JSON 数据模型，而非传统的关系型数据模型。

JSON 数据结构的全称为 JavaScript Object Notation，是一种轻量级的数据交换格式，非常易于人阅读和编写，同时也易于机器生成和解析。

它基于“JavaScript Programming Language, Standard ECMA-262 3rd Edition – December 1999”的一个子集，为纯文本格式，支持嵌套结构与数组。

**JSON 建构基于两种结构：**

*   **键值对集合**：在键值对集合结构中，每一个数据元素拥有一个键（名称）与一个值。值可以包含数字，字符串等常用结构，或嵌套 JSON 对象和数组。
*   **数组**：在数组中的每一个元素不包含元素名，其值可以为数字，字符串等常用结构，或者嵌套 JSON 对象和数组。

**JSON 具有如下形式：**

*   对象是一个无序的“键值对”集合，以“{”（左大括号）开始，“}”（右大括号）结束。每一个元素名后跟一个“:”（冒号）；而元素之间使用“,”（逗号）分隔；

    ![JSON对象](data_model/sequoiadb_datamodel_jsonstruct_img1.jpg)

*   数组是值的有序集合，以“[”（左中括号）开始，“]”（右中括号）结束。值之间使用“,”（逗号）分隔；

    ![JSON数组](data_model/sequoiadb_datamodel_jsonstruct_img2.jpg)

*   值可以为由双引号包裹的字符串，数值，对象，数组，true，false，null，以及SequoiaDB 数据库特有的数据结构（例如日期，时间等）组成。

    ![JSON值](data_model/sequoiadb_datamodel_jsonstruct_img3.jpg)

>   **Note:**
>
>   更多 JSON 的内容请参考 [介绍 JSON](http://json.org/json-zh.html)

##例子##

一个典型的 JSON 嵌套式数据结构如下：

![嵌套式数据结构](data_model/sequoiadb_datamodel_jsonstruct_img4.jpg)
