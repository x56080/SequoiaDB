/******************************************************************************
 * @Description   : seqDB-30163:外部模式删除字段默认值，字段存在写默认值
 * @Author        : liuli
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30163";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 101 } };
testConf.clName = COMMCLNAME + "_30163";
testConf.clOpt = { EnableInfoSchema: true };

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var schema = testPara.testSchema;
   var writeDefault = 101;

   dbcl.addSchema( testConf.schemaName );

   // 插入数据，不包含 b 字段
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: writeDefault } );
   }
   dbcl.insert( docs );

   var expInternalColumnDef = { a: {}, b: { WriteDefault: writeDefault } };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 删除字段默认值
   schema.dropColumnDefault( "b" );

   // 新插入数据不包含 b 字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 删除一个不存在字段的默认值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.dropColumnDefault( "c" );
   } );

   // 校验数据和内部模式结构
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var expInternalColumnDef = { a: {}, b: {} };
   checkInternalSchema( dbcl, expInternalColumnDef );
}