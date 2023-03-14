/******************************************************************************
 * @Description   : seqDB-30157:外部模式重命名索引字段
 * @Author        : liuli
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
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
      function( dbcl, schemaName )
      {
         dbcl.alter( { EnableInfoSchema: true, ReplSize: 0 } );
         dbcl.addSchema( schemaName );
      } );

   // 测试开启内部模式后插入数据
   testSchema(
      function( dbcs, clName, schemaName )
      {
         var dbcl = dbcs.createCL( clName, { EnableInfoSchema: true, ReplSize: 0 } );
         dbcl.addSchema( schemaName );
         return dbcl;
      },
      function()
      { } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30157";
   var schemaName = "schema_30157";
   var indexName1 = "index_30157_1";
   var indexName2 = "index_30157_2";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" }, "c": { Type: "int32" }, "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，部分数据包含外部模式中的字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i, d: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // a 字段创建索引
   dbcl.createIndex( indexName1, { a: 1 } );

   // b，c字段创建复合索引
   dbcl.createIndex( indexName2, { b: 1, c: 1 } );

   // 重命名 a 字段为 d 字段
   schema.renameColumn( "a", "d" );

   // 重命名 c 字段为 a 字段
   schema.renameColumn( "c", "a" );

   // 校验主备节点数据一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expResult );

   // 查询访问计划
   checkExplain( dbcl, { d: 5 }, "ixscan", indexName1 );
   checkExplain( dbcl, { a: 5, b: 5 }, "ixscan", indexName2 );

   // 检查主备节点索引一致性
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName2, true );

   commDropCL( db, COMMCSNAME, clName );
}