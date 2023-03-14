/******************************************************************************
 * @Description   : seqDB-30112:删除已绑定的外部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var mainCLName = "mainCL_30112";
   var clName = "cl_30112";
   var schemaName1 = "schema_30112_1";
   var schemaName2 = "schema_30112_2";
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );

   commCreateSchema( db, schemaName1, schemaDef );
   commCreateSchema( db, schemaName2, schemaDef );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { EnableInfoSchema: true } );

   // 普通表绑定外部模式
   dbcl.addSchema( schemaName1 );

   // 删除普通表绑定的外部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      db.dropSchema( schemaName1 );
   } );

   // 主表绑定外部模式
   maincl.addSchema( schemaName2 );

   // 删除主表绑定的外部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      db.dropSchema( schemaName2 );
   } );

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, clName );
}