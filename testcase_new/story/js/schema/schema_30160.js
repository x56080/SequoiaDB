/******************************************************************************
 * @Description   : seqDB-30160:主子表分区键不同，重命名分区键字段
 * @Author        : liuli
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_30160";
   var subCLName1 = "subCL_30160_1";
   var subCLName2 = "subCL_30160_2";
   var schemaName = "schema_30160";

   // 建表绑定开启内部模式绑定外部模式
   testSchema(
      function()
      {
         var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
         var subclOptions = { ShardingKey: { b: 1 }, ShardingType: "hash", EnsureShardingIndex: false, AutoSplit: true, EnableInfoSchema: true };
         commCreateCL( db, COMMCSNAME, subCLName1, subclOptions );
         commCreateCL( db, COMMCSNAME, subCLName2, subclOptions );
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
         var subclOptions = { ShardingKey: { b: 1 }, ShardingType: "hash", EnsureShardingIndex: false, AutoSplit: true };
         commCreateCL( db, COMMCSNAME, subCLName1, subclOptions );
         commCreateCL( db, COMMCSNAME, subCLName2, subclOptions );
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

   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var maincl = func1();

   // 插入数据，均为外部模式中存在的字段
   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   maincl.insert( docs );

   func2( maincl );

   // 外部模式重命名主表 ShardingKey 字段
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.renameColumn( "a", "c" );
   } );

   // 外部模式重命名子表 ShardingKey 字段
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      schema.renameColumn( "b", "d" );
   } );

   // 校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   commDropCL( db, COMMCSNAME, mainCLName );
}