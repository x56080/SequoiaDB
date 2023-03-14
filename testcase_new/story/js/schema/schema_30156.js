/******************************************************************************
 * @Description   : seqDB-30156:外部模式重命名字段和以存在字段计算hash值相同
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
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
   var clName = "cl_30156";
   var schemaName = "schema_30156";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   // 创建外部模式
   var writeDefault = { "$date": "2011-01-01" };
   var readDefault = { "$date": "2011-02-01" };
   var schemaDef = {
      field1: { Type: "int32", WriteDefault: 10, ReadDefault: 20 }, b: { Type: "date", WriteDefault: writeDefault, ReadDefault: readDefault }, c: { Type: "double", WriteDefault: 2.47, ReadDefault: 6.48 }, d: { Type: "string", WriteDefault: "default", ReadDefault: "default" }, "e": {
         Type: "int64", WriteDefault: 1000000000, ReadDefault: 2000000000
      }
   };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var expResult = [];
   var docs = [];
   var expPrimalResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { field1: i, b: "2022-10-01", c: 1.27, d: "infoSchema", e: i * 1000000000 } );
      expResult.push( { field1: i, ygdgoabx: "2022-10-01", kqxa: 1.27, ejrwex: "infoSchema", xitgkjd: i * 1000000000 } );
      expPrimalResult.push( { field1: i, ygdgoabx: "2022-10-01", kqxa: 1.27, ejrwex: "infoSchema", xitgkjd: i * 1000000000 } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   schema.renameColumn( "b", "ygdgoabx" );
   schema.renameColumn( "c", "kqxa" );
   schema.renameColumn( "d", "ejrwex" );
   schema.renameColumn( "e", "xitgkjd" );

   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { field1: i, ygdgoabx: "2023-02-01" } );
      expResult.push( { field1: i, ygdgoabx: "2023-02-01", kqxa: 2.47, ejrwex: "default", xitgkjd: 1000000000 } );
      expPrimalResult.push( { field1: i, ygdgoabx: "2023-02-01", kqxa: 2.47, ejrwex: "default", xitgkjd: 1000000000 } );
   }
   dbcl.insert( docs );

   // 校验贴源、非贴源数据
   var actResult = dbcl.find().sort( { field1: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { field1: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   commDropCL( db, COMMCSNAME, clName );
}