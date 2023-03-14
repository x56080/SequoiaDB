/******************************************************************************
 * @Description   : seqDB-30126:不存在数据的集合绑定外部模式，单字段设置读默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30126";
   var mainCLName = "maincl_30126_";
   var subCLName1 = "subcl_30126_1";
   var subCLName2 = "subcl_30126_2";
   var schemaName = "schema_30126";
   commDropSchema( db, schemaName );

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   var schemaDef = { "d": { Type: "int32" }, "b": { Type: "string", ReadDefault: "infoSchema" } };
   commCreateSchema( db, schemaName, schemaDef );
   maincl.addSchema( schemaName );

   // 插入数据包含外部模式字段
   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { d: i, a: i } );
      expResult.push( { d: i, a: i, b: "infoSchema" } );
      expPrimalResult.push( { d: i, a: i } );
   }
   maincl.insert( docs );

   // 校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 插入数据不包含外部模式字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: "infoSchema" } );
      expPrimalResult.push( { a: i } );
   }
   maincl.insert( docs );

   // 校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   commDropCS( db, csName );
}