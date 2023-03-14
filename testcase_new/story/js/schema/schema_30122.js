/******************************************************************************
 * @Description   : seqDB-30122:未绑定外部模式，子表存在数据，主表关闭内部模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.03.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30122";
   var mainCLName = "maincl_30122_";
   var subCLName1 = "subcl_30122_1";
   var subCLName2 = "subcl_30122_2";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   // 插入数据
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: { "$date": "2022-10-01" } } );
   }
   maincl.insert( docs );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.alter( { EnableInfoSchema: false } );
   } )
}