/******************************************************************************
 * @Description   : seqDB-31637:$concat 其他类型自动转化为字符串
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.18
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31637";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   dbcl.insert( { No: 1, a: "abc" } );

   // { <字段名> : { $concat : "<字符串>" } }
   // <字符串>为数字
   var actRecords = dbcl.find( {}, { "a": { "$concat": 123 } } );
   var expRecords = [{ No: 1, a: "abc123" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为布尔值
   actRecords = dbcl.find( {}, { "a": { "$concat": true } } );
   expRecords = [{ No: 1, a: "abctrue" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为Date
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$date": "2022-01-01" } } } );
   expRecords = [{ No: 1, a: "abc2022-01-01" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为timestamp
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$timestamp": "2012-01-01-13.14.26.124233" } } } );
   expRecords = [{ No: 1, a: "abc2012-01-01-13.14.26.124233" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为decimal
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$decimal": "123.456312313131313131" } } } );
   expRecords = [{ No: 1, a: "abc123.456312313131313131" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为OID
   actRecords = dbcl.find( {}, { "a": { "$concat": ObjectId( "55713f7953e6769804000001" ) } } );
   expRecords = [{ No: 1, a: "abc55713f7953e6769804000001" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为Object
   actRecords = dbcl.find( {}, { "a": { "$concat": { "b": "value" } } } );
   expRecords = [{ No: 1, a: "abc{ \"b\": \"value\" }" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>含有特殊字符
   actRecords = dbcl.find( {}, { "a": { "$concat": "?!@#$%\\|\"" } } );
   expRecords = [{ No: 1, a: "abc?!@#$%\\|\"" }];
   commCompareResults( actRecords, expRecords );

   // <字符串>含有中文
   actRecords = dbcl.find( {}, { "a": { "$concat": "中文" } } );
   expRecords = [{ No: 1, a: "abc中文" }];
   commCompareResults( actRecords, expRecords );
}