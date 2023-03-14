/******************************************************************************
 * @Description   : seqDB-30102:绑定外部模式的主表挂载为开启内部模式的子表
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30102";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "string", WriteDefault: "infoSchema" } };

main( test );
function test ( testPara )
{
   var csName = "cs_30102";
   var mainCLName = "maincl_30102";
   var subCLName1 = "subcl_30102_1";
   var subCLName2 = "subcl_30102_2";
   var subCLName3 = "subcl_30102_3";

   commDropCS( db, csName );

   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   // 挂载子表
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   maincl.addSchema( testConf.schemaName );

   commCreateCL( db, csName, subCLName3, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.attachCL( csName + "." + subCLName3, { LowBound: { a: 1000 }, UpBound: { a: 1500 } } );
   } )

   commDropCS( db, csName );
}