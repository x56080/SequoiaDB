/******************************************************************************
 * @Description   : seqDB-30348:主子表属于不同CS，删除主表所在CS，绑定外部模式不存在默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCSName = "mainCS_30348";
   var subCSName = "subCS_30348";
   var mainCLName = "mainCL_30348";
   var subCLName1 = "subCL_30348_1";
   var subCLName2 = "subCL_30348_2";
   var schemaName = "schema_30348";
   db.getRecycleBin().dropAll();
   commDropCS( db, subCSName );
   commDropCS( db, mainCSName );
   commDropSchema( db, schemaName );
   db.getRecycleBin().dropAll();

   // 挂载子表，部分子表与主表不在同一CS
   var maincl = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, mainCSName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   var subcl2 = commCreateCL( db, subCSName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( mainCSName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   //   创建外部模式，字段不存在默认值
   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   // 主表绑定外部模式
   maincl.addSchema( schemaName );

   // 插入数据
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   maincl.insert( docs );

   for( var i = 100; i < 200; i++ )
   {
      expResult.push( { a: i, b: i } );
   }

   // 删除主表所在CS
   commDropCS( db, mainCSName );

   expResult.sort( sortBy( "a" ) );
   var actResult = subcl2.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 创建主表开启内部模式
   var maincl = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );

   //   挂载剩余的子表
   // assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   // {
   //    maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   // } )
   maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   db.getRecycleBin().dropAll();
   commDropCS( db, mainCSName );
   commDropCS( db, subCSName );
}