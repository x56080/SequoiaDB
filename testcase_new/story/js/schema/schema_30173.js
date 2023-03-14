/******************************************************************************
 * @Description   : seqDB-30173：删除字段后添加同名字段设置写默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.22
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
//main( test );

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
   var clName = "cl_30173";
   var schemaName = "schema_30173";
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
      doc.push( { a: i } );
      expResult.push( {} );
   }
   dbcl.insert( doc );

   // add schema
   func2( dbcl, schemaName );
   checkAddSchema( db, COMMCSNAME, clName, schemaName );

   // insert data
   doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i } );
      expResult.push( {} );
   }
   dbcl.insert( doc );

   // schema drop column
   schema.dropColumn( "a" );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // schema add column
   schema.addColumn( "a", { Type: "int32", WriteDefault: 10 } );

   // insert data
   doc = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { b: i } );
      expResult.push( { b: i, a: 10 } );
   }
   dbcl.insert( doc );

   expResult.sort( sortBy( 'a' ) );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // check internal schema
   var expInternalColumnDef = {
      "a": {
         "WriteDefault": 10
      },
      "b": {}
   }
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}
