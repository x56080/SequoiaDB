/******************************************************************************
 * @Description   : seqDB-30205:$rename修改外部模式字段，字段无默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.27
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
         var dbcl = dbcs.createCL( clName, { EnableInfoSchema: true, ReplSize: 0 } );
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
   var clName = "cl_30205";
   var schemaName = "schema_30205";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { a: { Type: "int32" }, b: { Type: "int64" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, c: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 再次插入部分数据
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, c: i } );
   }
   dbcl.insert( docs );

   // 使用 $rename 更新 b 字段为 c
   dbcl.update( { $rename: { "b": "c" } } )

   // 校验主备节点数据一致性
   var expInternalColumnDef = { a: {}, b: {}, c: {} };
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}