/******************************************************************************
 * @Description   : seqDB-30203:绑定外部模式的集合truncate
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30203";
testConf.schemaDef = { "b": { Type: "int32", ReadDefault: 10 } };
testConf.clName = COMMCLNAME + "_30203";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   // add schema
   testPara.testCL.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // insert data
   var doc = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
   }
   testPara.testCL.insert( doc );
   testPara.testCL.truncate();

   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
      primalResult.push( { a: i } );
   }
   testPara.testCL.insert( doc );

   var actResult = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   commDropCL( db, COMMCSNAME, testConf.clName );
}
