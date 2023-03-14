/******************************************************************************
 * @Description   : seqDB-30194：主子表在不同CS绑定外部模式，删除主表所在CS
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCSName = "mainCS_30194";
   var subCSName = "subCS_30194";
   var mainCLName = "mainCL_30194";
   var subCLName1 = "subCL_30194_1";
   var subCLName2 = "subCL_30194_2";
   var schemaName = "schema_30194";
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
   var subcl2 = commCreateCL( db, subCSName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( mainCSName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   // 插入数据
   var docs = [];
   for( var i = 0; i < 120; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   maincl.insert( docs );

   // 删除主表所在CS
   commDropCS( db, mainCSName );

   // 子表插入数据
   var docs = [];
   var expResult = [];
   for( var i = 100; i < 150; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   subcl2.insert( docs );
   for( var i = 100; i < 120; i++ )
   {
      expResult.push( { a: i, b: i } );
   }

   expResult.sort( sortBy( "a" ) );
   var actResult = subcl2.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   commDropCS( db, mainCSName );
   commDropCS( db, subCSName );
}

