/******************************************************************************
 * @Description   : seqDB-30206：$rename修改外部模式字段，字段写默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30206";
testConf.schemaDef = { "b": { Type: "string", WriteDefault: "write" } };
testConf.clName = COMMCLNAME + "_30206";
testConf.clOpt = { EnableInfoSchema: true };
// main( test );

function test ( testPara )
{
   testPara.testCL.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 插入数据
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { a: i, b: "insertString" } );
   }
   testPara.testCL.insert( docs );

   // $rename修改字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      testPara.testCL.update( { $rename: { "b": "updateColumnName" } } );
   } )

   // 插入数据
   expResult = docs;
   docs = [];
   for( var i = 20; i < 40; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: "write" } );
   }
   testPara.testCL.insert( docs );

   // 校验数据
   var actResult = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );
}
