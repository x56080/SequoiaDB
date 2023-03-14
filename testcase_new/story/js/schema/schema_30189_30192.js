/******************************************************************************
 * @Description   : seqDB-30189:集合绑定外部模式删除集合
 *                  seqDB-30192:集合空间下多个集合绑定外部模式，删除集合空间
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30189_30192";
   var clName1 = "cl_30189_30192_1";
   var clName2 = "cl_30189_30192_2";
   var clName3 = "cl_30189_30192_3";
   var schemaName1 = "schema_30189_30192_1";
   var schemaName2 = "schema_30189_30192_2";
   var schemaName3 = "schema_30189_30192_3";

   commDropCS( db, csName );
   var options = { EnableInfoSchema: true };
   var dbcl1 = commCreateCL( db, csName, clName1, options );
   var dbcl2 = commCreateCL( db, csName, clName2, options );
   var dbcl3 = commCreateCL( db, csName, clName3, options );

   // 创建多个外部模式
   var schemaDef1 = { "a": { Type: "int32" } };
   var schemaDef2 = { "b": { Type: "string", WriteDefault: "default", ReadDefault: "default" } };
   commCreateSchema( db, schemaName1, schemaDef1 );
   commCreateSchema( db, schemaName2, schemaDef2 );
   commCreateSchema( db, schemaName3, schemaDef2 );

   dbcl1.addSchema( schemaName1 );
   dbcl2.addSchema( schemaName2 );
   dbcl3.addSchema( schemaName3 );

   commDropCL( db, csName, clName1 );
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName1 );
   } )

   commDropCS( db, csName );
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName1 );
   } )
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName2 );
   } )
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName3 );
   } )
}