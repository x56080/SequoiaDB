/******************************************************************************
 * @Description   : seqDB-30169:字段重命名后，外部模式删除贴源字段
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
   var clName = "cl_30169";
   var schemaName = "schema_30169";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var writeDefault = 1000;
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: writeDefault } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult1 = [];
   var expResult2 = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult1.push( { a: i, c: i } );
      expResult2.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 外部模式字段重命名
   schema.renameColumn( "b", "c" );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult1 );

   // 校验外部模式属性
   var expColumnDef = { "a": { Type: "int32" }, "c": { Type: "int32", WriteDefault: writeDefault } };
   checkColumnDef( db, schemaName, expColumnDef );

   // 再次插入数据为重命名后的字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, c: i } );
      expResult1.push( { a: i, c: i } );
      expResult2.push( { a: i } );
   }
   for( var i = 200; i < 300; i++ )
   {
      docs.push( { a: i } );
      expResult1.push( { a: i, c: writeDefault } );
      expResult2.push( { a: i } );
   }
   dbcl.insert( docs );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult1 );

   // 外部模式删除字段
   schema.dropColumn( "c" );

   // 校验主备节点数据一致性
   var expInternalColumnDef = { a: {} };
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult2, expResult2, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}