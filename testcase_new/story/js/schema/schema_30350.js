/******************************************************************************
 * @Description   :  seqDB-30350:绑定外部模式的主表detachCL，外部模式不存在默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.07
 * @LastEditTime  : 2023.03.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30350";
   var mainCLName = "maincl_30350_";
   var subCLName1 = "subcl_30350_1";
   var schemaName = "schema_30350";
   commDropCS( db, csName );
   commDropSchema( db, schemaName );

   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   var subcl1 = commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );

   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );
   maincl.addSchema( schemaName );

   // 插入数据
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
   }
   maincl.insert( docs );

   //主表detach子表
   maincl.detachCL( csName + "." + subCLName1 );

   // 子表查询数据
   var actResult = subcl1.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   // 主表重新挂载子表
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );

   commDropCS( db, csName );
   commDropSchema( db, schemaName );
}