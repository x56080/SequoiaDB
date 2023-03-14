/******************************************************************************
 * @Description   : seqDB-30174:删除存在索引的字段
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
   var clName = "cl_30174";
   var schemaName = "schema_30174";
   var indexName = "index_30174";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   // 创建外部模式
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // 集合插入数据，均为外部模式中存在的字段
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // 创建复合索引
   dbcl.createIndex( indexName, { 'a': 1, 'b': 1 } );

   // 外部模式删除一个索引字段，失败报错
   var schema = db.getSchema( schemaName );
   // assert.tryThrow( SDB_INVALIDARG, function()
   // {
   //    schema.dropColumn( "b" );
   // } )

   dbcl.dropIndex( indexName );
   schema.dropColumn( "b" );

   // 创建同名索引
   dbcl.createIndex( indexName, { 'a': 1, 'b': 1 } );

   // 校验数据并查询访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   checkExplain( dbcl, { a: 5 }, "ixscan", indexName );

   commDropCL( db, COMMCSNAME, clName );
}