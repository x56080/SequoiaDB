/******************************************************************************
 * @Description   : seqDB-30154:外部模式字段重命名，修改后的字段在外部模式和记录中均已存在
 * @Author        : liuli
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
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
         dbcl.alter( { EnableInfoSchema: true, ReplSize: 0 } );
         dbcl.addSchema( schemaName );
      } );

   // 测试开启内部模式绑定外部模式后插入数据
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
   var clName = "cl_30154";
   var schemaName = "schema_30154";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var readDefault = 1000;
   var writeDefault = 2000;
   var schemaDef = { "a": { Type: "int32" }, "c": { Type: "int32" }, "b": { Type: "int32", ReadDefault: readDefault, WriteDefault: writeDefault } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，部分数据包含外部模式中的字段
   var docs = [];
   for( var i = 0; i < 50; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 外部模式字段重命名，重命名后的字段在外部模式中已存在
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.renameColumn( "c", "b" );
   } );

   // 校验主备节点数据一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, docs, docs );

   commDropCL( db, COMMCSNAME, clName );
}