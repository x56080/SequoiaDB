/******************************************************************************
 * @Description   : seqDB-30357:主表绑定外部模式，恢复dropCL项目
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.07
 * @LastEditTime  : 2023.03.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30357";
   cleanRecycleBin( db, csName );
   var mainCLName = "maincl_30357";
   var subCLName1 = "subcl_30357_1";
   var subCLName2 = "subcl_30357_2";
   var schemaName = "schema_30357";
   var schemaName2 = "schema_30351_2";
   var schemaName3 = "schema_30351_3";
   commDropCS( db, csName );
   commDropSchema( db, schemaName );
   commDropSchema( db, schemaName2 );
   commDropSchema( db, schemaName3 );

   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   var schemaDef = { "d": { Type: "int32", ReadDefault: 10, WriteDefault: 20 }, "b": { Type: "string", ReadDefault: "infoSchema", WriteDefault: "test" } };
   commCreateSchema( db, schemaName, schemaDef );
   maincl.addSchema( schemaName );

   // 插入数据
   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { d: i, a: i } );
      expResult.push( { d: i, a: i, b: "test" } );
      expPrimalResult.push( { d: i, a: i } );
   }
   maincl.insert( docs );

   // 删除主表后进行恢复
   var dbcs = db.getCS( csName );
   dbcs.dropCL( mainCLName );
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   var schemaDef2 = { "d": { Type: "int32", ReadDefault: 100, WriteDefault: 200 }, "b": { Type: "string", ReadDefault: "read", WriteDefault: "write" } };
   commCreateSchema( db, schemaName2, schemaDef2 );

   maincl.addSchema( schemaName2 );

   // commCreateSchema( db, schemaName3, schemaDef );
   // maincl.addSchema( schemaName3 );

   commDropCS( db, csName );
   commDropSchema( db, schemaName );
   commDropSchema( db, schemaName2 );
   commDropSchema( db, schemaName3 );
   cleanRecycleBin( db, csName );
}