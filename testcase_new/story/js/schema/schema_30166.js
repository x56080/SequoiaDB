/******************************************************************************
 * @Description   : seqDB-30166:外部模式删除贴源字段
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.02.24
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
   var clName = "cl_30166";
   var schemaName = "schema_30166";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 再次插入一段数据
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 外部模式删除 b 字段
   schema.dropColumn( "b" );

   // 校验外部模式属性
   var expColumnDef = { "a": { Type: "int32" } };
   checkColumnDef( db, schemaName, expColumnDef );

   // 校验主备节点数据一致性
   var expInternalColumnDef = { a: {} };
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}