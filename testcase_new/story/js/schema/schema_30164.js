/******************************************************************************
 * @Description   : seqDB-30164:外部模式删除字段默认值，字段存在读写默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.22
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30164";
testConf.schemaDef = { "a": { Type: "int32", ReadDefault: 10, WriteDefault: 20 } };
testConf.clName = COMMCLNAME + "_30164";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   // insert data
   var dbcl = testPara.testCL;
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { b: i } );
      expResult.push( { a: 10, b: i } );
      primalResult.push( { b: i } );
   }
   dbcl.insert( doc );

   dbcl.addSchema( testConf.schemaName );

   // delete column's default value
   testPara.testSchema.dropColumnDefault( "a" );

   doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { b: i } );
      // only writeDefult can be deleted
      expResult.push( { a: 10, b: i } );
      primalResult.push( { b: i } );
   }
   dbcl.insert( doc );

   var actResult = dbcl.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // check internal schema
   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 10
      },
      "b": {}
   }
   checkInternalSchema( dbcl, expInternalColumnDef );
}
