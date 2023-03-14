/******************************************************************************
 * @Description   : seqDB-30161：分区表修改字段为分区键字段
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30161";
testConf.schemaDef = { "a": { Type: "bool", WriteDefault: false } };
testConf.clName = COMMCLNAME + "_30161";
testConf.clOpt = { ShardingKey: { age: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   testPara.testCL.addSchema( testConf.schemaName );
   // 写入数据包含外部模式字段：a
   var doc = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
   }
   testPara.testCL.insert( doc );

   // 重命名字段a为分区键字段
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      testPara.testSchema.renameColumn( "a", "age" );
   } );

   var actResult = testPara.testCL.find().sort( { b: 1 } );
   commCompareResults( actResult, doc );
   actResult = testPara.testCL.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, doc );
}