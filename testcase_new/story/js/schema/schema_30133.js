/******************************************************************************
 * @Description   : seqDB-30133:集合已有数据不包含外部模式字段，外部模式字段设置读默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.28
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
   var clName = "cl_30133";
   var schemaName = "schema_30133";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
   commCreateSchema( db, schemaName, schemaDef );

   // 插入数据，数据不包含外部模式中的一个字段
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
   }
   dbcl.insert( docs );

   func2( dbcl );

   // 绑定外部模式增加字段b包含读默认值
   dbcl.addSchema( schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 校验主备节点数据一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, docs );

   commDropCL( db, COMMCSNAME, clName );
}