/******************************************************************************
 * @Description   : seqDB-30197:集合存在贴源/非贴源数据执行split
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.23
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30197";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30197";
testConf.clOpt = { ShardingKey: { a: 1 }, "ShardingType": "hash", EnableInfoSchema: true };
testConf.useDstGroup = true;
testConf.useSrcGroup = true;

main( test );

function test ( testPara )
{
   var schemaName = "schema_30197";
   commDropSchema( db, schemaName );
   var dbcl = testPara.testCL;
   var schema = testPara.testSchema;

   // add schema
   dbcl.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // insert data
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
      primalResult.push( { a: i } );
   }
   dbcl.insert( doc );

   // schema add column
   schema.addColumn( "b", { Type: "int32", ReadDefault: 10 } );

   // insert data
   var doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
      primalResult.push( { a: i } );
   }
   dbcl.insert( doc );

   // split
   dbcl.split( testPara.srcGroupName, testPara.dstGroupNames[0], 50 );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   dbcl.split( testPara.srcGroupName, testPara.dstGroupNames[1], 50 );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   commDropCL( db, COMMCSNAME, testConf.clName );
}
