/******************************************************************************
 * @Description   : seqDB-30103:子表绑定外部模式
 *                : seqDB-30120:主表绑定外部模式，子表关闭内部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var mainCLName = "mainCL_30103_30120";
   var subCLName1 = "subCL_30103_30120_1";
   var subCLName2 = "subCL_30103_30120_2";
   var schemaName1 = "schema_30103_30120_1";
   var schemaName2 = "schema_30103_30120_2";
   var writeDefault = true;
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "bool", WriteDefault: writeDefault } };

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );

   commCreateSchema( db, schemaName1, schemaDef );
   commCreateSchema( db, schemaName2, schemaDef );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   var subcl1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 子表绑定外部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      subcl1.addSchema( schemaName1 );
   } );

   // 主表绑定外部模式
   maincl.addSchema( schemaName1 );

   // 子表绑定另一个外部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      subcl1.addSchema( schemaName2 );
   } );

   // 子表关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      subcl1.alter( { EnableInfoSchema: false } );
   } );

   // 主表插入数据
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: writeDefault } );
   }
   maincl.insert( docs );

   // 子表关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      subcl1.alter( { EnableInfoSchema: false } );
   } );

   // 校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName2 );
}