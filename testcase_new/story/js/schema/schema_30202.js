/******************************************************************************
 * @Description   : seqDB-30202:绑定外部模式的集合重命名
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 开启内部模式插入数据后绑定外部模式
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
   var clName = "cl_30202";
   var clNameNew = "cl_30202_new";
   var schemaName = "schema_30202";

   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, clNameNew );
   commDropSchema( db, schemaName );

   var schemaDef = { b: { Type: "int32" }, c: { Type: "int64" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i, c: i * 1000000000 } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 集合重命名
   dbcs.renameCL( clName, clNameNew );

   // 校验数据
   dbcl = dbcs.getCL( clNameNew );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, clNameNew, schemaName );
   var cursor = db.list( SDB_LIST_SCHEMAS, { Name: schemaName, Collection: COMMCSNAME + "." + clNameNew } );
   if( !cursor.next() )
   {
      throw new Error( "no corresponding schema found!" );
   }
   cursor.close();

   // 删除集合
   dbcs.dropCL( clNameNew );

   // 检查外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName );
   } );
}