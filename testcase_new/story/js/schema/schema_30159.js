/******************************************************************************
 * @Description   : seqDB-30159:分区表重命名ShardingKey
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 测试插入数据后开启内部模式绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, {
            "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoSplit": true,
            EnsureShardingIndex: false, EnableInfoSchema: true
         } );
      },
      function( dbcl, schemaName )
      {
         dbcl.alter( { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
      } );

   // 测试开启内部模式后插入数据绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, {
            "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoSplit": true, EnsureShardingIndex: false,
            EnableInfoSchema: true, EnableInfoSchema: true
         } );
      },
      function( dbcl, schemaName )
      {
         dbcl.addSchema( schemaName );
      } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30159";
   var schemaName = "schema_30159";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   //  创建外部模式
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "string" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: "string" } );
      expResult.push( { a: i, b: "string" } );
      expPrimalResult.push( { a: i, b: "string" } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.renameColumn( "a", "c" );
   } )

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );
}