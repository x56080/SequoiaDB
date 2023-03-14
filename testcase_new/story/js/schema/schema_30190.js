/******************************************************************************
 * @Description   : seqDB-30190:主表绑定外部模式删除主表
 * @Author        : liuli
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var dbcs = testPara.testCS;
   var mainCLName1 = "mainCL_30190_1";
   var mainCLName2 = "mainCL_30190_2";
   var subCLName = "subCL_30190";
   var schemaName1 = "schema_30190_1";
   var schemaName2 = "schema_30190_2";
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };

   commDropCL( db, COMMCSNAME, mainCLName1 );
   commDropCL( db, COMMCSNAME, mainCLName2 );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );

   var maincl1 = commCreateCL( db, COMMCSNAME, mainCLName1, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName, { ShardingKey: { b: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   var maincl2 = commCreateCL( db, COMMCSNAME, mainCLName2, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateSchema( db, schemaName1, schemaDef );
   commCreateSchema( db, schemaName2, schemaDef );

   // 2个主表均绑定 Schema
   maincl1.addSchema( schemaName1 );
   maincl2.addSchema( schemaName2 );

   // 主表 maincl1 挂载子表
   maincl1.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   // 删除2个主表
   dbcs.dropCL( mainCLName1 );
   dbcs.dropCL( mainCLName2 );

   // 检查 Schema 一并被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName1 );
   } );
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName2 );
   } );
}