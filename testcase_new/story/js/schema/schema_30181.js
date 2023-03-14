/******************************************************************************
 * @Description   : seqDB-30181:非贴源字段创建索引指定NotArray为true，读默认值为数组
 * @Author        : liuli
 * @CreateTime    : 2023.02.23
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
   var clName = "cl_30181";
   var schemaName = "schema_30181";
   var indexName = "index_30181";
   var readDefault = ["readDefault"];

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { a: { Type: "int32" }, b: { Type: "int32", WriteDefault: 101 } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: readDefault } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 外部模式增加字段，读默认值为 array 类型
   var columnDef = { Type: "array", WriteDefault: ["writeDefault"], ReadDefault: readDefault };
   schema.addColumn( "c", columnDef );

   // 指定新增字段创建索引，指定NotArray为true
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      dbcl.createIndex( indexName, { c: 1 }, { NotArray: true } );
   } );

   // 校验主备节点数据一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, docs );

   commDropCL( db, COMMCSNAME, clName );
}