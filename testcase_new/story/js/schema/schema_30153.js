/******************************************************************************
 * @Description   : seqDB-30153:外部模式字段重命名，修改后的字段在外部模式中已存在
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.07
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
   var clName = "cl_30153";
   var schemaName = "schema_30153";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 10, WriteDefault: 20 }, "b": { Type: "string", WriteDefault: "infoSchema" }
   };

   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i } );
      expPrimalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.renameColumn( "a", "b" );
   } )

   // 校验数据
   var actResult = dbcl.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { b: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 10,
         "WriteDefault": 20
      },
      "b": {
         "WriteDefault": "infoSchema"
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}