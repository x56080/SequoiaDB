##语法##
***db.createCS(&lt;name&gt;,[options])***

在数据库对象中创建集合空间。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 集合空间名。同一个数据库对象中，集合空间名必须唯一。 | 是 |
| options | Json | 对象 | 集合空间可选属性。 | 否 |

###options 格式##

| 属性名 | 描述 | 格式 |
| ------ | ------ | ------ |
| PageSize | 数据页大小。默认为65536B。 | PageSize:&lt;int32&gt; |
| Domain | 所属域 | Domain:&lt;string&gt; |
| LobPageSize | Lob数据页大小。默认262144B | LobPageSize:&lt;int32&gt; |
| IndexEngineType（社区版） | 索引存储引擎类型。默认mmap | IndexEngineType:&lt;string&gt; |

**Note:**

* name 字段的值不能是空串，含点（.）或者美元符号（$）。且长度不超过127B。
* 同一个数据库对象集合空间名必须唯一。
* 在创建集合空间时用户可以指定数据页大小，指定后不可更改。如果不指定默认为65536B。
* PageSize 只能选填0，4096，8192，16384，32768，65536之一，0即为默认值65536。
* 所属域必须已经存在，且不能为 SYSDOMAIN。
* 为兼容较早版本接口，db.createCS(&lt;name&gt;,[ PageSize ]) 同样可以工作。
* LobPageSize只能选填0，4096，8192，16384，32768，65536，131072，262144，524288之一，0即为默认值262144。
* IndexEngineType可选填mmap。

##示例##

* 创建名为 foo 的集合空间，不指定数据页大小，即数据页大小为默认值65536B

<pre class="prettyprint lang-javascript">
> db.createCS("foo")</pre>

* 创建名为 foo 的集合空间，指定数据页大小为4096B，所属域为“mydomain”
<pre class="prettyprint lang-javascript">
> db.createCS("foo",{PageSize:4096,Domain:"mydomain"})</pre>

