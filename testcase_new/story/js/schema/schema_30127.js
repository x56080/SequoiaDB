/******************************************************************************
 * @Description   : seqDB-30127 :: 版本: 1 :: 不存在数据的集合绑定外部模式，单字段设置读写默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.03
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30127";
testConf.schemaDef = { "a": { Type: "double", ReadDefault: 2.12, WriteDefault: 2.12 } };
testConf.clName = COMMCLNAME + "_30127";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( testPara )
{
   dbcl = testPara.testCL;
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: 4.57, b: i } );
      expResult.push( { a: 4.57, b: i } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { b: i } );
      expResult.push( { a: 2.12, b: i } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );
}