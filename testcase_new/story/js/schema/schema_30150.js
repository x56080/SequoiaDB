/******************************************************************************
 * @Description   : seqDB-30150:外部模式增加字段计算hash相同
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.23
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
   var clName = "cl_30150";
   var schemaName = "schema_30150";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   // 创建外部模式
   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var schema = db.getSchema( schemaName );
   schema.addColumn( "field1", { Type: "int32" } );
   schema.addColumn( "ygdgoabx", { Type: "int32" } );
   schema.addColumn( "kqxa", { Type: "string", WriteDefault: "default", ReadDefault: "default" } );
   schema.addColumn( "ejrwex", { Type: "int32", WriteDefault: 10 } );
   schema.addColumn( "xitgkjd", { Type: "int32", ReadDefault: 20 } );

   var expectedColumnDef = {
      "a": {
         "Type": "int32",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "field1": {
         "Type": "int32",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "ygdgoabx": {
         "Type": "int32",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "kqxa": {
         "Type": "string",
         "ReadDefault": "default",
         "WriteDefault": "default",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "ejrwex": {
         "Type": "int32",
         "WriteDefault": 10,
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "xitgkjd": {
         "Type": "int32",
         "ReadDefault": 20,
         "Restrict": 0,
         "RestrictDesc": ""
      }
   };

   // 校验外部模式增加字段成功
   checkColumnDef( db, schemaName, expectedColumnDef )
   // 插入数据
   var expResult = [];
   var expPrimalResult = [];
   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { field1: i, ygdgoabx: i } );
      expResult.push( { field1: i, ygdgoabx: i, kqxa: "default", xitgkjd: 20 } );
      expPrimalResult.push( { field1: i, ygdgoabx: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 校验贴源、非贴源数据
   var actResult = dbcl.find().sort( { field1: 1 } );
   commCompareResults( actResult, expResult );
   // var actResult = dbcl.find().sort( { field1: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   // commCompareResults( actResult, expPrimalResult );

   // 再次插入一段数据，检测默认值生效
   var docs = [];
   for( var i = 10; i < 20; i++ )
   {
      docs.push( { field1: i, ygdgoabx: i } );
      expResult.push( { field1: i, ygdgoabx: i, kqxa: "default", ejrwex: 10, xitgkjd: 20 } );
      expPrimalResult.push( { field1: i, ygdgoabx: i, kqxa: "default", xitgkjd: 20 } );
   }
   dbcl.insert( docs );

   // 校验贴源、非贴源数据
   var actResult = dbcl.find().sort( { field1: 1 } );
   commCompareResults( actResult, expResult );
   // var actResult = dbcl.find().sort( { field1: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   // commCompareResults( actResult, expexpPrimalResult );

   commDropCL( db, COMMCSNAME, clName );
}