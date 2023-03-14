/******************************************************************************
 * @Description   : seqDB-30138:集合所有集合均包含外部模式定义字段，字段设置读默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
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
   var clName = "cl_30138";
   var schemaName = "schema_30138";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: i, c: "test" } );
      expResult.push( { a: i, b: i, c: "test" } );
      expPrimalResult.push( { a: i, b: i, c: "test" } );
   }
   dbcl.insert( docs );

   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 10, WriteDefault: 20 }, "b": { Type: "int32", ReadDefault: 100, WriteDefault: 200 },
      "c": { Type: "string", ReadDefault: "c1", WriteDefault: "c2" }
   };
   commCreateSchema( db, schemaName, schemaDef );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // 读数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   docs = [];
   for( var i = 10; i < 20; i++ )
   {
      docs.push( { a: i, b: i, c: "inSchema" } );
      expResult.push( { a: i, b: i, c: "inSchema" } );
      expPrimalResult.push( { a: i, b: i, c: "inSchema" } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 10,
         "WriteDefault": 20
      },
      "b": {
         "ReadDefault": 100,
         "WriteDefault": 200
      },
      "c": {
         "ReadDefault": "c1",
         "WriteDefault": "c2"
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}