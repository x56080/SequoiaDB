/******************************************************************************
 * @Description   : seqDB-30146:存在设置读默认值和写默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.22
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // 测试插入数据后开启内部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { ReplSize: 0 } );
      },
      function( dbcl )
      {
         dbcl.alter( { EnableInfoSchema: true } );
      }
   );

   // 测试开启内部模式后插入数据
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true, ReplSize: 0 } );
      },
      function() { }
   );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30146";
   var schemaName = "schema_30146";
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var expResult = [];
   var primalResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: 10 } );
      primalResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // create schema
   var schemaDef = { "a": { Type: "int32" } };
   var schema = db.createSchema( schemaName, schemaDef );
   checkColumnDef( db, schemaName, schemaDef );

   // add schema
   func2( dbcl );
   dbcl.addSchema( schemaName );
   checkAddSchema( db, COMMCSNAME, clName, schemaName );

   // insert data
   var doc = [];
   for( var i = 100; i < 120; i++ )
   {
      doc.push( { a: i, b: i, c: i, d: i, e: i } );
      expResult.push( { a: i, b: i, c: i, d: i, e: i } );
      primalResult.push( { a: i, b: i, c: i, d: i, e: i } );
   }
   for( var i = 120; i < 140; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i, c: i } );
      primalResult.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( doc );

   // schema add column:c,d
   var cColumnDef = { Type: "int32", ReadDefault: 10, };
   schema.addColumn( "c", cColumnDef );
   var dColumnDef = { Type: "int32", WriteDefault: 20, };
   schema.addColumn( "d", dColumnDef );

   // inser data
   doc = [];
   for( var i = 140; i < 150; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i, c: i, d: 20 } );
      primalResult.push( { a: i, b: i, c: i, d: 20 } );
   }
   for( var i = 150; i < 160; i++ )
   {
      doc.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: 10, d: 20 } );
      primalResult.push( { a: i, b: i, d: 20 } );
   }
   dbcl.insert( doc );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // check internal schema
   var expInternalColumnDef = {
      "a": {},
      "b": {},
      "c": {
         "ReadDefault": 10
      },
      "d": {
         "WriteDefault": 20
      },
      "e": {}
   }
   checkInternalSchema( dbcl, expInternalColumnDef );

   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, primalResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}
