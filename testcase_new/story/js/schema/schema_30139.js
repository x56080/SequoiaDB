/******************************************************************************
 * @Description   : seqDB-30139:集合所有集合均包含外部模式定义字段，字段设置写默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 测试插入数据后开启内部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName );
      },
      function( dbcl )
      {
         dbcl.alter( { EnableInfoSchema: true } );
      } );

   // 测试开启内部模式后插入数据
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true } );
      },
      function()
      { } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30139";
   var schemaName = "schema_30139";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );
   var writeDefault = 2000;
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: writeDefault } };
   commCreateSchema( db, schemaName, schemaDef );

   // 插入数据，包含外部模式中的所有字段
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 500; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   func2( dbcl );

   // 集合绑定外部模式
   dbcl.addSchema( schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 再次插入数据，包含外部模式所有字段
   docs = [];
   for( var i = 500; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 再次插入数据，不包含外部模式中设置写默认值的字段
   docs = [];
   for( var i = 1000; i < 1500; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: writeDefault } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 校验内部模式
   var expInternalColumnDef = { "a": {}, "b": { WriteDefault: writeDefault } };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}