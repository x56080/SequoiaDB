/******************************************************************************
 * @Description   : seqDB-30137：集合所有集合均包含外部模式定义字段，字段无默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : Cheng Jingjing
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
         dbcl.alter( { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
      }
   );

   // 测试开启内部模式后插入数据绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true } );
      },
      function( dbcl, schemaName )
      {
         dbcl.addSchema( schemaName );
      }
   );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30137";
   var schemaName = "schema_30137";
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   // 创建集合
   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // 创建外部模式
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "decimal" } };
   db.createSchema( schemaName, schemaDef );
   checkColumnDef( db, schemaName, schemaDef );

   // 插入数据
   var doc = [];

   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i, b: { $deciaml: "1.001" } } );
   }
   dbcl.insert( doc );

   // 绑定外部模式
   func2( dbcl, schemaName );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, doc );
   actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, doc );

   // 插入数据
   var expResult = doc;
   doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i, b: { $deciaml: "1.001" } } );
      expResult.push( { a: i, b: { $deciaml: "1.001" } } );
   }
   dbcl.insert( doc );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   var expInternalColumnDef = {
      "a": {},
      "b": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );
   commDropCL( db, COMMCSNAME, clName );
}