/******************************************************************************
 * @Description   : seqDB-30110:创建同名外部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var schemaName = "schema_30110";

   commDropSchema( db, schemaName );
   var schemaDef1 = { "a": { Type: "int32", ReadDefault: 1 }, "b": { Type: "int32", WriteDefault: 1 } };
   commCreateSchema( db, schemaName, schemaDef1 );

   // 创建同名 schema
   var schemaDef2 = { "c": { Type: "int32" }, "d": { Type: "int32", ReadDefault: 1, WriteDefault: 1 } };
   assert.tryThrow( SDB_SCHEMA_EXIST, function()
   {
      db.createSchema( schemaName, schemaDef2 );
   } );

   checkColumnDef( db, schemaName, schemaDef1 );

   commDropSchema( db, schemaName );
}