/******************************************************************************
 * @Description   : seqDB-30211:外部模式中包含自增字段Generated取值为strict
 * @Author        : liuli
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.03
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
   var clName = "cl_30211";
   var schemaName = "schema_30211";
   var readDefault = 1000;
   var writeDefault = 2000;

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = {
      a: { Type: "int32" },
      b: { Type: "int32", ReadDefault: readDefault, WriteDefault: writeDefault }
   };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, schemaName );

   dbcl.createAutoIncrement( { Field: "b", Generated: "strict" } );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult = [];
   for( var i = 1; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 插入数据不包含自增字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 插入数据包含自增字段
   docs = [];
   for( var i = 200; i < 300; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 校验主备节点数据一致性
   var expInternalColumnDef = { a: {}, b: { ReadDefault: readDefault, WriteDefault: writeDefault } };
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}