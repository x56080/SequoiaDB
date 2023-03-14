/******************************************************************************
 * @Description   : seqDB-30167:外部模式删除非贴源字段
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
   var clName = "cl_30167";
   var schemaName = "schema_30167";
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // create schema
   var schemaDef = { "a": { Type: "int32" } };
   var schema = db.createSchema( schemaName, schemaDef );
   checkColumnDef( db, schemaName, schemaDef );

   // insert data
   var docs = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i } );
      primalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // add schema
   func2( dbcl, schemaName );

   // insert data
   docs = [];
   for( var i = 20; i < 40; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i } );
      primalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // schema add column
   schema.addColumn( "b", { Type: "int32", ReadDefault: 10 } );
   schema.dropColumn( "b" );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // check internal schema
   var expInternalColumnDef = {
      "a": {}
   }
   checkInternalSchema( dbcl, expInternalColumnDef );
   commDropCL( db, COMMCSNAME, clName );
}
