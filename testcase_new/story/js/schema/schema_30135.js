/******************************************************************************
 * @Description   : seqDB-30135:集合部分数据包含外部模式字段，外部模式字段设置读写默认值读写数据
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
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
   var clName = "cl_30135";
   var schemaName = "schema_30135";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // 插入数据，数据部分包含外部模式，部分不包含
   docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: i, c: "inSchema" + i } );
      expResult.push( { a: i, b: i, c: "inSchema" + i } );
      expPrimalResult.push( { a: i, b: i, c: "inSchema" + i } );
   }
   for( var i = 10; i < 20; i++ )
   {
      docs.push( { a: i, c: "inSchema" + i } );
      expResult.push( { a: i, c: "inSchema" + i, b: 100 } );
      expPrimalResult.push( { a: i, c: "inSchema" + i } );
   }

   dbcl.insert( docs );

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 100, WriteDefault: 200 }, "c": { Type: "string" } };
   commCreateSchema( db, schemaName, schemaDef );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 写入数据包含该字段
   var record = { a: 446, b: 446, c: 446 };
   dbcl.insert( record );

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   expResult.push( { a: 446, b: 446, c: 446 } );
   expPrimalResult.push( { a: 446, b: 446, c: 446 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 写入数据不包含该字段
   record = { a: 500, c: 500 };
   dbcl.insert( record );

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   expResult.push( { a: 500, b: 200, c: 500 } );
   expPrimalResult.push( { a: 500, b: 200, c: 500 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   var expInternalColumnDef = {
      "b": {
         "ReadDefault": 100,
         "WriteDefault": 200
      },
      "a": {},
      "c": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 检验主备一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expPrimalResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}