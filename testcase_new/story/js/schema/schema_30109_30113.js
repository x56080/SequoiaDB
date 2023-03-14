/******************************************************************************
 * @Description   : seqDB-30109：创建外部模式，包含一个字段
 *                  seqDB-30113：删除/获取不存在的外部模式
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.21
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30109_30113";
testConf.schemaDef = { "a": { Type: "int32", ReadDefault: 1 } };
main( test );

function test ()
{
   checkColumnDef( db, testConf.schemaName, testConf.schemaDef );

   // 删除创建的外部模式
   db.dropSchema( testConf.schemaName );

   // 获取不存在的外部模式
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } );

   // 删除不存在的外部模式
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.dropSchema( testConf.schemaName );
   } );

   // 创建同名外部模式
   schemaDef = { "b": { Type: "int32", ReadDefault: 10 } };
   db.createSchema( testConf.schemaName, schemaDef );

   checkColumnDef( db, testConf.schemaName, schemaDef );
}

