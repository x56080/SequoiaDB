/******************************************************************************
 * @Description   : seqDB-30195:主子表在不同CS绑定外部模式，删除子表所在CS
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCSName = "mainCS_30195";
   var subCSName = "subCS_30195";
   var mainCLName = "mainCL_30195";
   var subCLName1 = "subCL_30195_1";
   var subCLName2 = "subCL_30195_2";
   var schemaName = "schema_30195";
   commDropCS( db, subCSName );
   commDropCS( db, mainCSName );
   commDropSchema( db, schemaName );

   // 创建主表绑定外部模式
   var maincl = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   maincl.addSchema( schemaName );

   // 挂载子表，部分子表与主表不在同一CS
   commCreateCL( db, mainCSName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, subCSName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( mainCSName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   // 插入数据
   var docs = [];
   for( var i = 60; i < 120; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   maincl.insert( docs );

   // 删除子表所在CS
   db.dropCS( subCSName );

   checkAddSchema( db, mainCSName, mainCLName, schemaName );

   // 主表插入数据
   docs = [];
   var expResult = [];
   for( var i = 0; i < 50; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   maincl.insert( docs );
   for( var i = 60; i < 100; i++ )
   {
      expResult.push( { a: i, b: i } );
   }

   expResult.sort( sortBy( "a" ) );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   commDropCS( db, mainCSName );
}
