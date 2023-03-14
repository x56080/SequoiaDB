/******************************************************************************
 * @Description   : seqDB-30172:删除字段后添加同名字段设置读默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.28
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
         var dbcl = dbcs.createCL( clName, { ReplSize: 0, EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
         return dbcl;
      },
      function()
      { } );

   // 插入数据后开启内部模式绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { ReplSize: 0 } );
      },
      function( dbcl, schemaName )
      {
         dbcl.alter( { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
      } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30172";
   var schemaName = "schema_30172";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 10, ReadDefault: 2000 } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，全部为外部模式中包含的字段
   var readDefault = 1000;
   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: readDefault } );
      expPrimalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 外部模式删除一个不存在的字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.dropColumn( "c" );
   } );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   // 外部模式删除字段
   schema.dropColumn( "b" );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expPrimalResult );

   // 外部模式新增同名字段设置读默认值
   schema.addColumn( "b", { Type: "int32", ReadDefault: readDefault } );

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 插入数据，不包含新增字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: readDefault } );
      expPrimalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 校验主备节点数据一致性
   var expInternalColumnDef = { a: {}, b: { ReadDefault: readDefault } };
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expPrimalResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}