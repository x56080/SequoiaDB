/******************************************************************************
 * @Description   : seqDB-30223：addColumn接口参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.02.28
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30223";
testConf.schemaDef = { "a": { Type: "int32" } };
main( test );

function test ( testPara )
{
   var schema = testPara.testSchema;

   // 1.columnName字段
   // 1.1 合法值
   schema.addColumn( "b", { Type: "int32" } );
   // 1.2 非string类型
   // assert.tryThrow( SDB_INVALIDARG, function()
   // {
   //    schema.addColumn( {}, { Type: "int32" } );
   // } );
   // 1.3 不填
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      schema.addColumn( { Type: "int32" } );
   } );

   // 2.columnDef字段
   // 2.1 合法值：object类型
   schema.addColumn( "c", { Type: "int32" } );
   // 2.2 指定不存在的type类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "xx", { Type: "int" } );
   } );
   // 2.3 指定非object类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "xx", "{}" );
   } );

   // 2.4 不填
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      schema.addColumn( "xxx" );
   } );
   // 2.5 type指定为非string类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "c", { Type: 32 } );
   } );
   // 2.6 指定取值列表以外的参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "c", { Type: "bool", WriteDefault: "notTrue" } );
   } );
   // 2.6 不指定type
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "c", { WriteDefault: "write" } );
   } );
   // 2.7 Restrict指定NotNull、NotArray、NotNull|NotArray
   schema.addColumn( "d", { Type: "int32", Restrict: "NotNull" } );
   schema.addColumn( "dd", { Type: "int32", Restrict: "NotArray" } );
   schema.addColumn( "ddd", { Type: "int32", Restrict: "NotNull|NotArray" } );
}
