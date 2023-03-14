/******************************************************************************
 * @Description   : seqDB-3025：dropColumnDefault接口参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.02.28
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30225";
testConf.schemaDef = { "a": { Type: "int32", WriteDefault: 10, ReadDefault: 20 } }
testConf.clName = COMMCLNAME + "_30117";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   var schema = testPara.testSchema;

   testPara.testCL.addSchema( testConf.schemaName );

   // 1.非法参数
   // 1.1 指定一个不存在的字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.dropColumnDefault( "columnName" );
   } );
   // 1.2 非string类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.dropColumnDefault( 123 );
   } );
   // 1.2 不指定
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      schema.dropColumnDefault();
   } );

   // 2. 合法参数
   schema.dropColumnDefault( "a" );
   var newColumnDef = { "a": { Type: "int32", "ReadDefault": 20 } };
   checkColumnDef( db, testConf.schemaName, newColumnDef );
}
