/******************************************************************************
 * @Description   : seqDB-30222:getSchema接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   // 不指定参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getSchema();
   } );

   // name指定为 int 类型
   var schemaName = 1;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getSchema( schemaName );
   } );

   // 指定 name 超过 127 字节
   schemaName = new Array( 128 ).join( "a" );
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName );
   } );
}