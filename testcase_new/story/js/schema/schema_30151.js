/******************************************************************************
 * @Description   : seqDB-30151:修改外部模式字段写默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 开启内部模式绑定外部模式后插入数据
   testSchema(
      function( dbcs, clName, schemaName )
      {
         var dbcl = dbcs.createCL( clName, { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
         return dbcl;
      },
      function()
      { } );

   // 插入数据后开启内部模式绑定外部模式
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
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30151";
   var schemaName = "schema_30151";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，不包含 b 字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 外部模式 b 字段增加写默认值
   var writeDefault = 101;
   schema.alterColumn( "b", { WriteDefault: writeDefault } );

   // 新插入数据不包含 b 字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: writeDefault } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 外部模式修改 b 字段写默认值，与原始值相同
   schema.alterColumn( "b", { WriteDefault: writeDefault } );
   docs = [];
   for( var i = 200; i < 300; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: writeDefault } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 外部模式修改 b 字段写默认值，与原始值不相同
   writeDefault = 201;
   schema.alterColumn( "b", { WriteDefault: writeDefault } );
   docs = [];
   for( var i = 300; i < 400; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: writeDefault } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 外部模式修改不存在字段的写默认值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.alterColumn( "c", { WriteDefault: writeDefault } );
   } );

   commDropCL( db, COMMCSNAME, clName );
}