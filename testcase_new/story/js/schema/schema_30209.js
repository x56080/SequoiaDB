/******************************************************************************
 * @Description   : seqDB-30209：外部模式中包含自增字段Generated取值为always
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30209";
testConf.schemaDef = { "b": { Type: "int32", ReadDefault: 10, WriteDefault: 20 } };
testConf.clName = COMMCLNAME + "_30209";
testConf.clOpt = { AutoIncrement: { Field: "b", Generated: "always" }, EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   testPara.testCL.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 插入数据
   var docs = [];
   var expResult = [];
   for( var i = 1; i < 20; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: i } );
   }
   testPara.testCL.insert( docs );

   docs = [];
   for( var i = 20; i < 40; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   testPara.testCL.insert( docs );

   // 校验数据
   var actResult = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );
}
