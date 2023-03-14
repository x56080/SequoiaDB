/******************************************************************************
 * @Description   : seqDB-30099:集合属性不支持绑定外部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var clName1 = "cl_30099_1";
   var clName2 = "cl_30099_2";
   var schemaName1 = "schema_30099_1";
   var schemaName2 = "schema_30099_2";
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 10 } };
   var dbcs = testPara.testCS;
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );

   commCreateSchema( db, schemaName1, schemaDef );
   commCreateSchema( db, schemaName2, schemaDef );

   // 创建集合不开启内部模式，绑定外部模式
   var dbcl1 = dbcs.createCL( clName1 );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl1.addSchema( schemaName1 );
   } );

   // 集合开启内部模式后绑定外部模式
   dbcl1.alter( { EnableInfoSchema: true } );
   dbcl1.addSchema( schemaName1 );

   // 集合绑定一个新的外部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl1.addSchema( schemaName2 );
   } );

   // 创建集合绑定一个已经被使用的外部模式
   var dbcl2 = dbcs.createCL( clName2, { EnableInfoSchema: true } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl2.addSchema( schemaName1 );
   } );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropSchema( db, schemaName2 );
}