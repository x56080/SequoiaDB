/******************************************************************************
 * @Description   : seqDB-30182:删除索引后字段保持贴源属性
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30182";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "double" } };
testConf.clName = COMMCLNAME + "_30182";
testConf.clOpt = { EnableInfoSchema: true };
// main( test );

function test ( testPara )
{
   var schema = db.getSchema( testConf.schemaName );
   checkColumnDef( db, testConf.schemaName, testConf.schemaDef );

   // 集合绑定外部模式
   testPara.testCL.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 插入数据包含外部模式所有字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { a: i, b: 1.001 } );
      expResult.push( { a: i, b: 1.001, subject: ["math", "english"] } );
   }
   testPara.testCL.insert( docs );
   // 外部模式增加字段设置读写默认值
   schema.addColumn( "subject", { Type: "array", ReadDefault: ["math", "english"], WriteDefault: ["chinese"] } );

   // 插入数据
   var primalResult = expResult;
   docs = [];
   for( var i = 20; i < 40; i++ )
   {
      docs.push( { a: i, subject: ["chinese"] } );
   }
   testPara.testCL.insert( docs );
   expResult = expResult.concat( docs );
   primalResult = primalResult.concat( docs );

   docs = [];
   for( var i = 40; i < 60; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, subject: ["chinese"] } );
      primalResult.push( { a: i, subject: ["chinese"] } );
   }
   testPara.testCL.insert( docs );

   // 新增字段创建索引
   var idxName = "index_30182";
   testPara.testCL.createIndex( idxName, { subject: 1 } );
   checkExplain( testPara.testCL, { subject: 1 }, "ixscan", idxName );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   println( actResult );
   // 删除索引
   testPara.testCL.dropIndex( idxName );

   // var actResult = testPara.testCL.find().sort( { a: 1 } );
   // commCompareResults( actResult, expResult );
   // actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   // commCompareResults( actResult, primalResult );
}
