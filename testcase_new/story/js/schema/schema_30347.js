/******************************************************************************
 * @Description   : seqDB-30347:主子表属于不同CS，删除主表所在CS，绑定外部模式存在默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCSName = "mainCS_30347";
   var subCSName = "subCS_30347";
   var mainCLName = "mainCL_30347";
   var subCLName1 = "subCL_30347_1";
   var subCLName2 = "subCL_30347_2";
   var schemaName1 = "schema_30347_1";
   var schemaName2 = "schema_30347_2";
   commDropCS( db, subCSName );
   commDropCS( db, mainCSName );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );
   db.getRecycleBin().dropAll();

   // 挂载子表，部分子表与主表不在同一CS
   var maincl = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { d: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, mainCSName, subCLName1, { ShardingKey: { d: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   var subcl2 = commCreateCL( db, subCSName, subCLName2, { ShardingKey: { d: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( mainCSName + "." + subCLName1, { LowBound: { d: 0 }, UpBound: { d: 100 } } );
   maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { d: 100 }, UpBound: { d: 200 } } );

   // 创建外部模式
   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 10, WriteDefault: 40 }, "b": { Type: "string", ReadDefault: "test" },
      "c": { Type: "double", WriteDefault: 2.48 }
   };
   commCreateSchema( db, schemaName1, schemaDef );

   // 主表绑定外部模式
   maincl.addSchema( schemaName1 );

   // 插入数据
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: "infoSchema", d: i } );
   }
   maincl.insert( docs );

   for( var i = 100; i < 200; i++ )
   {
      expResult.push( { a: i, b: "infoSchema", d: i, c: 2.48 } );
   }

   // 删除主表所在CS
   commDropCS( db, mainCSName );

   // 子表查询数据
   expResult.sort( sortBy( "a" ) );
   var actResult = subcl2.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   var schemaDef = {
      "f": { Type: "int32", ReadDefault: 30, WriteDefault: 50 }
   };
   commCreateSchema( db, schemaName2, schemaDef );

   // 绑定失败
   // assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   // {
   //    subcl2.addSchema( schemaName2 );
   // } )
   // subcl2.addSchema( schemaName2 );

   // 创建主表开启内部模式
   var maincl = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { d: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );

   // 挂载剩余的子表
   // assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   // {
   //    maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { d: 100 }, UpBound: { d: 200 } } );
   // } )
   db.getRecycleBin().dropAll();
   commDropCS( db, mainCSName );
   commDropCS( db, subCSName );
}
