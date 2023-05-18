/******************************************************************************
 * @Description   : seqDB-31639:$concat 拼接字符串包含特殊字符、中文、空格等
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.18
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31639";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   var docs = [
      { No: 1, a: "?!@#$%\\|\"" },
      { No: 2, a: "巨杉" },
      { No: 3, a: " " }
   ];
   dbcl.insert( docs );

   // { <字段名> : { $concat : "<字符串>" } }
   // <字符串>含有特殊字符
   actRecords = dbcl.find( {}, { "a": { "$concat": "?\\\"" } } );
   expRecords = [
      { No: 1, a: "?!@#$%\\|\"?\\\"" },
      { No: 2, a: "巨杉?\\\"" },
      { No: 3, a: " ?\\\"" }
   ];
   commCompareResults( actRecords, expRecords );

   // <字符串>含有中文
   actRecords = dbcl.find( {}, { "a": { "$concat": "数据库" } } );
   expRecords = [
      { No: 1, a: "?!@#$%\\|\"数据库" },
      { No: 2, a: "巨杉数据库" },
      { No: 3, a: " 数据库" }
   ];
   commCompareResults( actRecords, expRecords );

   // <字符串>含有空格
   actRecords = dbcl.find( {}, { "a": { "$concat": " " } } );
   expRecords = [
      { No: 1, a: "?!@#$%\\|\" " },
      { No: 2, a: "巨杉 " },
      { No: 3, a: "  " }
   ];
   commCompareResults( actRecords, expRecords );
}