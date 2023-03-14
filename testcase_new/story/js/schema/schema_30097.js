/******************************************************************************
 * @Description   : seqDB-30097:创建分区表绑定外部模式，不开启内部模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.02
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var clName1 = "cl_30097_1";
   var clName2 = "cl_30097_2";
   var schemaName = "schema_30097";

   commDropSchema( db, schemaName );

   // 创建外部模式
   var schemaDef = { b: { Type: "int64" } };
   commCreateSchema( db, schemaName, schemaDef );

   var options = { ShardingKey: { a: 1 } };
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1, options );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl1.addSchema( schemaName );
   } )

   options = { ShardingKey: { a: 1 }, EnableInfoSchema: false };
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, options );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl2.addSchema( schemaName );
   } )

   commDropSchema( db, schemaName );
}