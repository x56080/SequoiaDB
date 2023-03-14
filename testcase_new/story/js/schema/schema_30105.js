/******************************************************************************
 * @Description   : seqDB-30105:主表未开启内部模式绑定外部模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30105";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "string", WriteDefault: "infoSchema" } };

main( test );
function test ()
{
   var csName = "cs_30105";
   var mainCLName = "maincl_30105";
   var subCLName1 = "subcl_30105_1";
   var subCLName2 = "subcl_30105_2";

   commDropCS( db, csName );

   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.addSchema( testConf.schemaName );
   } )

   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 5 }, UpBound: { a: 10 } } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.addSchema( testConf.schemaName );
   } )

   commDropCS( db, csName );
}