/******************************************************************************
 * @Description   : seqDB-30094:创建主表时绑定外部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30094";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 10 } };

main( test );
function test ( testPara )
{
   var clName = "maincl_30094";
   var dbcs = testPara.testCS;
   commDropCL( db, COMMCSNAME, clName );

   // 创建主表不指定EnableInfoSchema，绑定外部模式
   var maincl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.addSchema( testConf.schemaName );
   } );
   dbcs.dropCL( clName );

   // 创建主表指定EnableInfoSchema为false，绑定外部模式
   var maincl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: false } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.addSchema( testConf.schemaName );
   } );
   dbcs.dropCL( clName );

   // 创建主表指定EnableInfoSchema为true，绑定外部模式
   var maincl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   maincl.addSchema( testConf.schemaName );

   // 校验主表和外部模式绑定关系
   var cursor = db.list( SDB_LIST_SCHEMAS, { Name: testConf.schemaName, Collection: COMMCSNAME + "." + clName } );
   if( !cursor.next() )
   {
      throw new Error( "no corresponding schema found!" );
   }
   cursor.close();
   checkAddSchema( db, COMMCSNAME, clName, testConf.schemaName );

   commDropCL( db, COMMCSNAME, clName );
}