/******************************************************************************
 * @Description   : seqDB-30168:外部模式删除字段，存在贴源记录和非贴源记录
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
   var clName = "cl_30168";
   var schemaName = "schema_30168";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );
   // 创建外部模式
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // 集合插入数据，均为外部模式中存在的字段
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // 外部模式新增字段设置读写默认值
   var schema = db.getSchema( schemaName );
   schema.addColumn( "c", { Type: "int32", ReadDefault: 200, WriteDefault: 10 } );

   // 插入数据，包含外部模式所有字段
   docs = [];
   for( var i = 10; i < 20; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 删除字段c，检查内部模式
   schema.dropColumn( "c" );
   var expInternalColumnDef = {
      "a": {},
      "b": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   var expInternalColumnDef = {
      "a": {},
      "b": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}