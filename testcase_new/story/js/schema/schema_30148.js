/******************************************************************************
 * @Description   : seqDB-30148:集合部分数据包含外部模式字段，外部模式字段设置读写默认值更新数据
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.22
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
   var clName = "cl_30148";
   var schemaName = "schema_30148";
   var indexName = "index_30148";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，所有记录均包含一个外部模式中不存在的字段 c
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // c 字段创建索引
   dbcl.createIndex( indexName, { c: 1 } );

   // 外部模式增加 c 字段无默认值
   schema.addColumn( "c", { Type: "int32" } );

   // 校验数据并查询访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { c: 5 }, "ixscan", indexName );

   // 外部模式 c 字段增加写默认值
   var writeDefault = 101;
   schema.alterColumn( "c", { WriteDefault: writeDefault } );

   // 新插入数据不包含 c 字段
   dbcl.insert( { a: 101, b: 101 } );
   docs.push( { a: 101, b: 101, c: writeDefault } );

   // 校验数据并查询访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { c: 101 }, "ixscan", indexName );

   commDropCL( db, COMMCSNAME, clName );
}