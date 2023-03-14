/******************************************************************************
 * @Description   : seqDB-30352:旧版本集合truncate，强制恢复truncate项目
 * @Author        : liuli
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30352";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
testConf.clName = COMMCLNAME + "_30352";

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcl = args.testCL;

   var docs = [];
   var expResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
   }
   dbcl.insert( docs );

   // 集合执行truncate
   dbcl.truncate();

   // 集合开启内部模式绑定外部模式
   dbcl.alter( { EnableInfoSchema: true } );
   dbcl.addSchema( testConf.schemaName );

   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   // 恢复truncate项目
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 校验外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } );

   // 再次创建同名外部模式
   db.createSchema( testConf.schemaName, testConf.schemaDef );

   // 集合开启内部模式
   dbcl.alter( { EnableInfoSchema: true } );
   dbcl.addSchema( testConf.schemaName );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );
}