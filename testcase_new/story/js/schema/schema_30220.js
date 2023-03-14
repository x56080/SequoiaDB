/******************************************************************************
 * @Description   : seqDB-30220:createSchema接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 10 } };

   // 不指定参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema();
   } );

   // name指定为 int 类型
   var schemaName = 1;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } );

   // 指定 name 超过 127 字节
   schemaName = new Array( 128 ).join( "a" );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } );

   // 指定 name 为 127 字节
   schemaName = new Array( 127 ).join( "a" );
   db.createSchema( schemaName, schemaDef );
   db.dropSchema( schemaName );

   // 不指定 schemaDef
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName );
   } );

   // 指定 schemaDef 非 object 类型
   var schemaDef = "{}";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } );

   // 指定一个不存在的字段
   // var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", Default: 10 } };
   // assert.tryThrow( SDB_INVALIDARG, function()
   // {
   //    db.createSchema( schemaName, schemaDef );
   // } );
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WDefault: 10 } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } );

   // 不指定 Type 字段
   var schemaDef = { "a": { WriteDefault: "int32" } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } );

   // Type 指定不支持的类型
   var schemaDef = { "a": { Type: "long" }, "b": { Type: "int32", WriteDefault: 10 } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } );
}