/******************************************************************************
 * @Description   : seqDB-30162:外部模式删除字段默认值，字段不存在写默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

//main( test );

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
      } );

   // 测试开启内部模式后插入数据
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true, ReplSize: 0 } );
      },
      function()
      { } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30162";
   var schemaName = "schema_30162";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = {
      "a": { Type: "int32" }, "b": { Type: "string", ReadDefault: "test" }
   };

   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var docs = [];
   var expResult = [];
   var priexpResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: "test" } );
      priexpResult.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );
   // throw ( e );

   var schema = db.getSchema( schemaName );
   schema.dropColumnDefault( "a" );
   schema.dropColumnDefault( "b" );


   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, priexpResult );

   var expectedColumnDef = {
      "a": {
         "Type": "int32",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "b": {
         "Type": "string",
         "ReadDefault": "test",
         "Restrict": 0,
         "RestrictDesc": ""
      }
   };
   checkColumnDef( db, schemaName, expectedColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}