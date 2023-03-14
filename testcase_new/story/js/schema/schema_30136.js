/******************************************************************************
 * @Description   : seqDB-30136:集合部分数据包含外部模式字段，外部模式字段设置读写默认值更新数据
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
   var clName = "cl_30136";
   var schemaName = "schema_30136";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );
   var readDefault = 1000;
   var writeDefault = 2000;
   var schemaDef = { "a": { Type: "int32" }, "c": { Type: "int32" }, "b": { Type: "int32", ReadDefault: readDefault, WriteDefault: writeDefault } };
   commCreateSchema( db, schemaName, schemaDef );

   // 插入数据，部分数据包含外部模式中的字段
   var expResult = [];
   var expUpdatedResult1 = [];
   var expUpdatedResult2 = [];
   var docs = [];
   for( var i = 0; i < 50; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
      expUpdatedResult1.push( { a: i, b: i } );
      expUpdatedResult2.push( { a: i, b: i } );
   }
   for( var i = 50; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: readDefault } );
      expUpdatedResult1.push( { a: i } );
      expUpdatedResult2.push( { a: i, b: readDefault + 1 } );
   }
   dbcl.insert( docs );

   // 集合开启内部模式
   func2( dbcl );

   // 绑定外部模式增加字段b包含读默认值
   dbcl.addSchema( schemaName );

   // 匹配不到记录upsert插入字段
   dbcl.upsert( { $set: { c: 10 } }, { a: 101 } );
   expResult.push( { a: 101, b: writeDefault, c: 10 } );
   expUpdatedResult1.push( { a: 101, b: writeDefault, c: 10 } );
   expUpdatedResult2.push( { a: 101, b: writeDefault, c: 10 } );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expUpdatedResult1 );

   // 修改所有读默认值记录
   dbcl.upsert( { $inc: { b: 1 } }, { b: readDefault } );

   // 校验主备节点数据一致性
   var expInternalColumnDef = { b: { ReadDefault: readDefault, WriteDefault: writeDefault }, a: {}, c: {} };
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expUpdatedResult2, expUpdatedResult2, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}