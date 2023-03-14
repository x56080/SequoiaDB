/******************************************************************************
 * @Description   : seqDB-30314:绑定外部模式的主表挂载子表
 * @Author        : liuli
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.02.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_30314";
   var subCLName1 = "subCL_30314_1";
   var subCLName2 = "subCL_30314_2";
   var schemaName = "schema_30314";

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   var schemaDef = { "b": { Type: "int32", WriteDefault: 20 }, "c": { Type: "int32", ReadDefault: 10 } };
   commCreateSchema( db, schemaName, schemaDef );
   maincl.addSchema( schemaName );

   // 新挂载一个子表
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 校验内部模式
   var expInternalColumnDef = { a: {}, b: { WriteDefault: 20 }, "c": { ReadDefault: 10 } };
   checkInternalSchema( maincl, expInternalColumnDef );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 20, c: 10 } );
      expPrimalResult.push( { a: i, b: 20 } );
   }
   maincl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}