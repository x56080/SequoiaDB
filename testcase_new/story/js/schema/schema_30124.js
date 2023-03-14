/******************************************************************************
 * @Description   : seqDB-30124:不存在数据的集合绑定外部模式，单字段不设置默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30124";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30124";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 集合绑定外部模式
   dbcl.addSchema( testConf.schemaName );

   // 插入数据包含外部模式字段
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 插入数据不包含外部模式字段
   dbcl.insert( { b: 1 } );

   // 贴源、非贴源校验数据
   var expResult = [{ b: 1 }];
   var actResult = dbcl.find( { b: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );
}