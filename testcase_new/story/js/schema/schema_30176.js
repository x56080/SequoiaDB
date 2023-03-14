/******************************************************************************
 * @Description   : seqDB-30176：更新数据匹配贴源字段
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.23
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30176";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30176";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   // create schema
   var schema = testPara.testSchema;
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
      primalResult.push( { a: i } );
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

   // update
   var cond = { a: 0 };
   testPara.testCL.upsert( { $set: { c: "ccc" } }, cond );
   expResult.splice( 0, 1, { a: 0, b: 10, c: "ccc" } );
   primalResult.splice( 0, 1, { a: 0, b: 10, c: "ccc" } );

   cond = { a: 1 };
   testPara.testCL.upsert( { $set: { b: 100 } }, cond );
   expResult.splice( 1, 1, { a: 1, b: 100, c: "cc" } );
   primalResult.splice( 1, 1, { a: 1, b: 100, c: "cc" } );

   var actResult = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // check internal schema
   var expInternalColumnDef = {
      "a": {},
      "b": {
         "ReadDefault": 10,
         "WriteDefault": 20
      },
      "c": {
         "ReadDefault": "cc"
      }
   }
   checkInternalSchema( testPara.testCL, expInternalColumnDef );

   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, testConf.clName, sel, expResult, primalResult, expInternalColumnDef );
}