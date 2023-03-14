/******************************************************************************
 * @Description   : seqDB-30142:增加所有数据均包含的字段，设置读写默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : liuli
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
      } );

   // 测试开启内部模式绑定外部模式后插入数据
   testSchema(
      function( dbcs, clName, schemaName )
      {
         var dbcl = dbcs.createCL( clName, { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
         return dbcl;
      },
      function()
      { } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30142";
   var schemaName = "schema_30142";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );
   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，c 字段是外部模式中不包含在字段
   var expResult = [];
   var expUpdatedResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i, c: "test" + i } );
      expResult.push( { a: i, b: i, c: "test" + i } );
      expUpdatedResult.push( { a: i, b: i, c: "test" + i } );
   }
   dbcl.insert( docs );

   // 集合开启内部模式
   func2( dbcl, schemaName );

   // 外部模式增加 c 字段，设置读写默认值
   var readDefault = "readSchema";
   var writeDefault = "writeSchema";
   schema.addColumn( "c", { Type: "string", ReadDefault: readDefault, WriteDefault: writeDefault } );

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 写入数据包含新增字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, b: i, c: "test" + i } );
      expResult.push( { a: i, b: i, c: "test" + i } );
      expUpdatedResult.push( { a: i, b: i, c: "writeSchema" } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 写入数据不包含新增字段
   docs = [];
   for( var i = 200; i < 300; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: "writeSchema" } );
      expUpdatedResult.push( { a: i, b: i, c: "writeSchema" } );
   }
   dbcl.insert( docs );

   // 校验贴源、非贴源数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 匹配不到记录upsert插入字段
   dbcl.upsert( { $set: { c: "readSchema" } }, { a: 260 } );
   var expResult = [{ a: 260, b: 260, c: "readSchema" }];
   var actResult = dbcl.find( { a: 260, b: 260 } ).sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find( { a: 260, b: 260 } ).sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 校验内部模式
   var expInternalColumnDef = { c: { ReadDefault: readDefault, WriteDefault: writeDefault }, a: {}, b: {} };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}