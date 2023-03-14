/******************************************************************************
 * @Description   : seqDB-3024：renameColumn接口参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.02.28
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30224";
testConf.schemaDef = { "a": { Type: "int32" } };
main( test );

function test ( testPara )
{
   var schema = testPara.testSchema;

   // 1.oldName字段
   // 1.1 合法值
   schema.renameColumn( "a", "b" );
   // 1.2 不指定
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      schema.renameColumn( "b" );
   } );
   // 1.3 指定一个不存在的字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.renameColumn( "oldName", "newName" );
   } );

   // 2.newName字段
   // 2.1 合法值
   schema.renameColumn( "b", "newName" );
   // 2.2 不指定
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      schema.renameColumn( "newName" );
   } );
   // 2.3 指定为非string类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "newName", 123 );
   } );
}
