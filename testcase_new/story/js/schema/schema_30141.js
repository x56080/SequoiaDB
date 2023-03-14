/******************************************************************************
 * @Description   : seqDB-30141:增加所有数据均不包含的字段，设置读写默认值
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
   var clName = "cl_30141";
   var schemaName = "schema_30141";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // 插入数据，数据包含外部模式所有字段
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 20 } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // 外部模式新增字段，设置默认读写
   var schema = db.getSchema( schemaName );
   schema.addColumn( "b", { Type: "int32", ReadDefault: 20, WriteDefault: 30 } );

   // 读数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 写入数据，包含外部模式新增字段
   var record = { a: 446, b: 446 };
   dbcl.insert( record );
   var actResult = dbcl.find().sort( { a: 1 } );
   expResult.push( { a: 446, b: 446 } );
   commCompareResults( actResult, expResult );

   // 写入数据，不包含外部模式新增字段
   record = { a: 500 };
   dbcl.insert( record );
   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   expResult.push( { a: 500, b: 30 } );
   commCompareResults( actResult, expResult );

   docs.push( { a: 446, b: 446 }, { a: 500, b: 30 } );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   var expInternalColumnDef = {
      "a": {},
      "b": {
         "ReadDefault": 20,
         "WriteDefault": 30
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 检验主备一致性
   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, expResult, docs, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}