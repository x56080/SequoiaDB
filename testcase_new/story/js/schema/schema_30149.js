/******************************************************************************
 * @Description   : seqDB-30149：外部模式增加字段存在读默认值，增加字段存在索引
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.22
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // 测试插入数据后开启内部模式绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName );
      },
      function( dbcl, schemaName )
      {
         dbcl.alter( { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
      }
   );

   // 测试开启内部模式后插入数据绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true } );
      },
      function( dbcl, schemaName )
      {
         dbcl.addSchema( schemaName );
      }
   );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30149";
   var schemaName = "schema_30149";
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // create schema
   var schemaDef = { "a": { Type: "int32" } };
   var schema = db.createSchema( schemaName, schemaDef );
   checkColumnDef( db, schemaName, schemaDef );

   // insert data
   var doc = [];
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( doc );

   func2( dbcl, schemaName );

   // insert data
   var doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( doc );

   // create index
   var idxName = "index_30149"
   dbcl.createIndex( idxName, { b: 1 }, true, true );

   // schema add column:b
   var columnDef = { Type: "int32", ReadDefault: 10 };
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.addColumn( "b", columnDef );
   } )
   columnDef = { Type: "int32", ReadDefault: 10, WriteDefault: 20, };
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.addColumn( "b", columnDef );
   } )

   // delete index
   dbcl.dropIndex( idxName );
   columnDef = { Type: "int32", ReadDefault: 10, WriteDefault: 20, };
   schema.addColumn( "b", columnDef );

   // create same index again
   dbcl.createIndex( idxName, { b: 1 }, true, true );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   checkExplain( dbcl, { b: 1 }, "ixscan", idxName );

   // check internal schema
   var expInternalColumnDef = {
      "a": {},
      "b": {
         "ReadDefault": 10,
         "WriteDefault": 20
      }
   }
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}
