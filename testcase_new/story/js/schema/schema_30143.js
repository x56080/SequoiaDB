/******************************************************************************
 * @Description   : seqDB-30143:增加部分数据包含的字段，设置读默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.21
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
      function()
      { }
   );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30143";
   var schemaName = "schema_30143";
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // create schema
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
   var schema = db.createSchema( schemaName, schemaDef );
   checkColumnDef( db, schemaName, schemaDef );

   // insert data
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i, c: i } );
      primalResult.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( doc );

   // add schema
   func2( dbcl );
   dbcl.addSchema( schemaName );
   checkAddSchema( db, COMMCSNAME, clName, schemaName );

   // insert data
   var doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: 10 } );
      primalResult.push( { a: i, b: i } );
   }
   for( var i = 40; i < 60; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i, c: i } );
      primalResult.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( doc );
   // check records
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, primalResult );

   // schema add column:c
   var cColumnDef = { Type: "int32", ReadDefault: 10 };
   schema.addColumn( "c", cColumnDef );

   // insert record inculde c
   var record = { a: 88, b: 88, c: 88 };
   expResult.push( record );
   primalResult.push( record );
   dbcl.insert( record );

   // insert record exclude c
   record = { a: 99, b: 99 };
   expResult.push( { a: 99, b: 99, c: 10 } );
   primalResult.push( record );
   dbcl.insert( record );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // upsert
   dbcl.upsert( { $set: { c: 100 } }, { a: 99 } );
   var expResult = [{ a: 99, b: 99, c: 100 }];
   var actResult = dbcl.find( { a: 99 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find( { a: 99 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // check internal schema
   var expInternalColumnDef = {
      "b": {
         "ReadDefault": 10
      },
      "a": {},
      "c": {
         "ReadDefault": 10
      }
   }
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}
