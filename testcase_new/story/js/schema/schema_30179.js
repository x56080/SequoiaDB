/******************************************************************************
 * @Description   : seqDB-30179:非贴源字段创建索引
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.23
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30179";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30179";
testConf.clOpt = { EnableInfoSchema: true };
// main( test );

function test ( testPara )
{
   // create schema
   var schema = db.getSchema( testConf.schemaName );
   checkColumnDef( db, testConf.schemaName, testConf.schemaDef );

   // add schema
   testPara.testCL.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // insert data
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
      expResult.push( { a: i, b: 10, c: "cc" } );
      primalResult.push( { a: i, b: 10 } );
   }
   testPara.testCL.insert( doc );

   // schema add column
   schema.addColumn( "b", { Type: "int32", ReadDefault: 10, WriteDefault: 20 } );
   schema.addColumn( "c", { Type: "string", ReadDefault: "cc" } );

   // insert data
   var doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i } );
      expResult.push( { a: i, b: 20, c: "cc" } );
      primalResult.push( { a: i, b: 20 } );
   }
   testPara.testCL.insert( doc );

   // create index
   var idxName = "index_30179";
   testPara.testCL.createIndex( idxName, { b: 1 } );

   var actResult = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // query access plan of mater and slave node
   var nodes = commGetCLNodes( db, COMMCSNAME + "." + testConf.clName );
   for( var i = 0; i < nodes.length; i++ )
   {
      var data = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
      try
      {
         var dbcl = data.getCS( COMMCSNAME ).getCL( testConf.clName );
         checkExplain( dbcl, { b: 1 }, "ixscan", idxName );
      } finally
      {
         data.close();
      }
   }
}
