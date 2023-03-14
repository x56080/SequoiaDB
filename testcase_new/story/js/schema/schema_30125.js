/******************************************************************************
 * @Description   : seqDB-30125：不存在数据的集合绑定外部模式，单字段设置写默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30125";
testConf.schemaDef = { "a": { Type: "bool", WriteDefault: false } };
testConf.clName = COMMCLNAME + "_30125";
testConf.clOpt = { ShardingKey: { age: 1 }, ShardingType: "hash", EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   // 绑定外部模式
   testPara.testCL.addSchema( testConf.schemaName );

   // 写入数据包含外部模式字段：a
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: true, b: i } );
      expResult.push( { a: true, b: i } );
      primalResult.push( { a: true, b: i } );
   }
   testPara.testCL.insert( doc );

   var actResult = testPara.testCL.find().sort( { b: 1 } );
   commCompareResults( actResult, doc );
   actResult = testPara.testCL.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, doc );

   // 写入数据不包含外部模式字段：a
   doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { b: i } );
      expResult.push( { b: i, a: false } );
      primalResult.push( { b: i, a: false } );
   }
   testPara.testCL.insert( doc );

   var actResult = testPara.testCL.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   actResult = testPara.testCL.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );
}