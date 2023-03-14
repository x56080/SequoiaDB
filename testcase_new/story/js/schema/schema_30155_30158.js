/******************************************************************************
 * @Description   : seqDB-30155：外部模式字段重命名，修改后字段在数据中已存在
 *                  seqDB-30158：外部模式字段重命名，重命名后的字段存在索引
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30155_30158";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30155";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   // 外部模式修改字段名
   var schema = db.getSchema( testConf.schemaName );
   schema.renameColumn( "a", "b" );
   var schemaDef = { "b": { Type: "int32" } }
   checkColumnDef( db, testConf.schemaName, schemaDef );

   testPara.testCL.addSchema( testConf.schemaName );
   // 插入数据
   var docs = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { b: i, c: i } );
   }
   testPara.testCL.insert( docs );

   // 将字段重命名为c
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.renameColumn( "b", "c" );
   } );

   // 重命名为索引字段
   var indexName = "dIdx";
   var key = { d: 1 }
   testPara.testCL.createIndex( indexName, key );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.renameColumn( "b", "d" );
   } );

   var actResult = testPara.testCL.find().sort( { b: 1 } );
   commCompareResults( actResult, docs );
   actResult = testPara.testCL.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );
}
