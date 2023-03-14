/******************************************************************************
 * @Description   : seqDB-30165:外部模式删除默认值后再新增默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
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
   var clName = "cl_30165";
   var schemaName = "schema_30165";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   // 创建外部模式，设置写默认值
   var schemaDef = { "a": { Type: "int32", WriteDefault: 10 } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var expResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { b: i } );
      expResult.push( { b: i } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   var actResult = dbcl.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );

   // 删除字段默认值
   schema.dropColumnDefault( "a" );
   // 设置新的默认值
   schema.alterColumn( "a", { Type: "int32", WriteDefault: 20 } );

   // 插入数据，不包含该字段，检验数据
   var record = { b: 446 };
   dbcl.insert( record );
   var actResult = dbcl.find().sort( { b: 1 } );
   expResult.push( { b: 446, a: 20 } )
   commCompareResults( actResult, expResult );

   // 检验内部模式
   var expInternalColumnDef = {
      "a": { "WriteDefault": 20 },
      "b": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}