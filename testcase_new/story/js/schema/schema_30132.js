/******************************************************************************
 * @Description   : seqDB-30132:集合已有数据不包含外部模式字段，外部模式字段无默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
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
   var clName = "cl_30132";
   var schemaName = "schema_30132";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var expResult = [];
   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 创建外部模式，外部模式字段不包含集合中的字段，均无默认值
   var schemaDef = { "c": { Type: "int32" }, "d": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   func2( dbcl );
   // 绑定外部模式
   dbcl.addSchema( schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 写入数据包含外部模式所有字段
   docs = [];
   for( var i = 10; i < 20; i++ )
   {
      docs.push( { a: i, b: i, c: i, d: i } );
      expResult.push( { a: i, b: i, c: i, d: i } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 校验内部模式结构
   var expInternalColumnDef = {
      "a": {},
      "b": {},
      "c": {},
      "d": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );
   // 检验主备一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, expResult, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}