1. 创建全文索引

	创建全文索引使用现有的语法结构，增加全文索引类型"text"，在索引的 key 定义中指定。索引的其它选项对全文索引无效，因此无需指定。以下语句在集合 foo.bar 的 name 及 address 字段上创建复合全文索引：

	```lang-javascript
	> db.foo.bar.createIndex('idx', {name:"text", address:"text"})
	```

	可指定一个或多个字段，需要注意的是在创建时 text 类型不可与其它任何类型混用。每创建一个全文索引，会在数据节点上对应地创建一个固定集合空间及固定集合（集合与集合空间同名，以 SYS_ 开头，与全文索引的对应关系可通过直连数据节点并使用 listIndexes() 进行查询）。文档在 Elasticsearch 中进行索引时，会使用原始集合中文档的 _id 字段的值生成 Elasticsearch 中文档的 _id，支持的原始文档的 _id 类型包括：

	- 32-bit integer
	- 64-bit integer
	- double
	- decimal
	- string
	- ObjectID
	- boolean
	- date
	- timestamp
	- Object

	全文索引在使用时存在以下约束：

    - 建议包含全文索引的集合中文档的 _id 值保持唯一，不唯一的情况下，在 Elasticsearch 上对应的文档不全，通过全文检索查询到的结果也会不全。这是由于在 Elasticsearch 的索引中，通过 _id 来唯一标识一个文档，而这个 _id 是通过 SequoiaDB 中文档的 _id 转换而来，对于 Elasticsearch，索引两个 _id 相同的文档，是进行一个 update 操作，先插入的文档会被覆盖。
	- 一个集合上最多创建 1 个全文索引
	- 数据库中最多创建 64 个全文索引
	- 只有字符串或字符串数组类型的字段会被索引，非字符串字段会被忽略
	- 不包含任何全文索引字段的文档将不会被索引，也无法被全文检索语法查询到（包括 Elasticsearch 中的 match_all 查询）
	- _id 字段类型不在受支持列表的文档不会被索引，也无法被全文检索语法查询到（包括 Elasticsearch 中的 match_all 查询）
	- SequoiaDB 记录中的 _id 在转换为 Elasticsearch 中的 _id 时进行了编码，由于 Elasticsearch 限制文档 _id 的最大长度为 512，只有 _id 值长度小于 254 字节的原始记录会被索引到 Elasticsearch 上，长度超过这个值的记录都会被忽略。基于性能方面的考虑，应尽量避免使用太长的 _id
	- 不包含全文检索语法中使用到的任何字段的记录无法被查询到

2. 删除全文索引

	删除全文索引使用 dropIndex 语法，指定索引名即可。

	```lang-javascript
	> db.foo.bar.dropIndex('idx')
	```
	在索引被删除时，其对应的固定集合空间也会一并删除。

3. 全文检索

	SequoiaDB 适配的全文检索引擎为 Elasticsearch，通过在 SequoiaDB 的查询语法中包含 Elasticsearch 的搜索条件来进行全文检索。基本语法结构为：

	```lang-javascript
	> db.foo.bar.find( { "" : { $Text : <search command> } } )
	```

	其中的 search command 即 Elasticsearch 的搜索条件。search command 部分支持 Elasticsearch 的 DSL（Domain Specific Language 特定领域语言）语句。因此，需要使用全文检索功能的用户，需要掌握 Elasticsearch 的 DSL 语言。

	以下示例程序使用全文索引对集合中 about 字段包含的"rock climbing"进行模糊查询：

	```lang-javascript
	> db.createCS('megacorp').createCL('employee')
	localhost:11810.megacorp.employee
	Takes 1.246399s.
	> db.megacorp.employee.createIndex('idx_1', {first_name:"text", "last_name":"text", "age":"text", "about":"text", "interests": "text"})
	Takes 1.182447s.
	> db.megacorp.employee.insert({"first_name" : "John","last_name" : "Smith","age" : 25,"about" : "I love to go rock climbing","interests": [ "sports", "music" ]})
	Takes 0.009290s.
	> db.megacorp.employee.insert({"first_name" : "Jane","last_name" : "Smith","age" : 32,"about" : "I like to collect rock albums","interests": [ "music" ]})
	Takes 0.001013s.
	> db.megacorp.employee.insert({"first_name" : "Douglas","last_name" : "Fir","age" : 35,"about": "I like to build cabinets","interests": [ "forestry" ]})
	Takes 0.001004s.
	> db.megacorp.employee.find({"":{"$Text":{"query":{"match":{"about" : "rock climbing"}}}}})
	{
	  "_id": {
		"$oid": "5a8f8d9c89000a0906000000"
	  },
	  "first_name": "John",
	  "last_name": "Smith",
	  "age": 25,
	  "about": "I love to go rock climbing",
	  "interests": [
		"sports",
		"music"
	  ]
	}
	{
	  "_id": {
		"$oid": "5a8f8d9f89000a0906000001"
	  },
	  "first_name": "Jane",
	  "last_name": "Smith",
	  "age": 32,
	  "about": "I like to collect rock albums",
	  "interests": [
		"music"
	  ]
	}
	Return 2 row(s).
	```
