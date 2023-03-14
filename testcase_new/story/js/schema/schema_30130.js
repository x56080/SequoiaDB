/******************************************************************************
 * @Description   : seqDB-30130:绑定存在字段名计算hash值相同的外部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : liuli
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
      } );

   // 测试开启内部模式后插入数据绑定外部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true } );
      },
      function( dbcl, schemaName )
      {
         dbcl.addSchema( schemaName );
      } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30130";
   var schemaName = "schema_30130";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = {
      field1: { Type: "int32" },
      ygdgoabx: { Type: "int32" },
      kqxa: { Type: "string", WriteDefault: "default", ReadDefault: "default" },
      ejrwex: { Type: "int32", WriteDefault: 10 },
      xitgkjd: { Type: "int32", ReadDefault: 20 }
   };
   commCreateSchema( db, schemaName, schemaDef );
   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // 插入数据
   var expResult = [];
   var expPrimalResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { field1: i, ygdgoabx: i } );
      expResult.push( { field1: i, ygdgoabx: i, kqxa: "default", xitgkjd: 20 } );
      expPrimalResult.push( { field1: i, ygdgoabx: i } );
   }
   dbcl.insert( docs );

   // 绑定外部模式
   func2( dbcl, schemaName );

   // 校验贴源、非贴源数据
   var actResult = dbcl.find().sort( { field1: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { field1: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 再次插入一段数据，检测默认值生效
   var docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { field1: i, ygdgoabx: i } );
      expResult.push( { field1: i, ygdgoabx: i, kqxa: "default", ejrwex: 10, xitgkjd: 20 } );
      expPrimalResult.push( { field1: i, ygdgoabx: i, kqxa: "default", ejrwex: 10 } );
   }
   dbcl.insert( docs );

   // 校验贴源、非贴源数据
   var actResult = dbcl.find().sort( { field1: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { field1: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   commDropCL( db, COMMCSNAME, clName );
}