/******************************************************************************
 * @Description   : seqDB-30144:增加字段设置写默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_30144";
   var subCLName1 = "subCL_30144_1";
   var subCLName2 = "subCL_30144_2";
   var schemaName = "schema_30144";

   // 建表绑定开启内部模式绑定外部模式
   testSchema(
      function()
      {
         var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
         commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
         commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
         maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
         maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
         maincl.addSchema( schemaName );
         return maincl;
      },
      function()
      { }, mainCLName, schemaName );

   // 建表不开启内部模式，插入数据后开启内部模式，绑定外部模式
   testSchema(
      function()
      {
         var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
         commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
         commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
         maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
         maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
         return maincl;
      },
      function( maincl )
      {
         maincl.alter( { EnableInfoSchema: true } );
         maincl.addSchema( schemaName );
      }, mainCLName, schemaName );
}

function testSchema ( func1, func2, mainCLName, schemaName )
{
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName );

   var schemaDef = { "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var maincl = func1();

   // 插入数据，均为外部模式中存在的字段
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   for( var i = 1000; i < 1100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i } );
   }
   maincl.insert( docs );

   func2( maincl );

   // 外部模式增加一个字段，设置写默认值
   var writeDefault = "writeSchema"
   schema.addColumn( "c", { Type: "string", WriteDefault: writeDefault } );

   // 再次写入数据，包含新增字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, b: i, c: "test" + i } );
      expResult.push( { a: i, b: i, c: "test" + i } );
   }
   for( var i = 1100; i < 1200; i++ )
   {
      docs.push( { a: i, b: i, c: "test" + i } );
      expResult.push( { a: i, b: i, c: "test" + i } );
   }
   maincl.insert( docs );

   // 校验贴源、非贴源数据
   expResult.sort( sortBy( "a" ) );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 再次写入数据，不包含新增字段
   docs = [];
   for( var i = 200; i < 300; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: "writeSchema" } );
   }
   for( var i = 1200; i < 1300; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: "writeSchema" } );
   }
   maincl.insert( docs );

   // 校验贴源、非贴源数据
   expResult.sort( sortBy( "a" ) );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 更新表1、表2非新增字段b数据
   maincl.upsert( { $set: { b: 60 } }, { a: 260 } );
   maincl.upsert( { $set: { b: 1160 } }, { a: 1260 } );
   var expResult1 = [{ a: 260, b: 60, c: "writeSchema" }];
   var actResult1 = maincl.find( { a: 260, b: 60 } ).sort( { a: 1 } );
   var expResult2 = [{ a: 1260, b: 1160, c: "writeSchema" }];
   var actResult2 = maincl.find( { a: 1260, b: 1160 } ).sort( { a: 1 } );
   commCompareResults( actResult1, expResult1 );
   commCompareResults( actResult2, expResult2 );
   var actResult1 = maincl.find( { a: 260, b: 60 } ).sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   var actResult2 = maincl.find( { a: 1260, b: 1160 } ).sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult1, expResult1 );
   commCompareResults( actResult2, expResult2 );

   // 校验内部模式
   var expInternalColumnDef = { a: {}, b: {}, c: { "WriteDefault": "writeSchema" } };
   checkInternalSchema( maincl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, mainCLName );
}