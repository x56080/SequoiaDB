/******************************************************************************
 * @Description   : seqDB-30353:存在数据冲突，强制恢复truncate项目
 * @Author        : liuli
 * @CreateTime    : 2023.03.07
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30353";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
testConf.clName = COMMCLNAME + "_30353";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcl = args.testCL;
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult1 = [];
   var expResult2 = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i } );
      expResult1.push( { a: i, b: 10 } );
      expResult2.push( { a: i, c: 20 } );
   }
   dbcl.insert( docs );

   // 集合执行truncate
   dbcl.truncate();

   // 再次插入数据
   dbcl.insert( { a: 1 } );

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
   commCompareResults( actResult, expResult1 );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 校验外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } );

   // 再次创建同名外部模式
   var schema = db.createSchema( testConf.schemaName, testConf.schemaDef );
   dbcl.addSchema( testConf.schemaName );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 外部模式新增字段并删除一个原有字段
   schema.addColumn( "c", { Type: "int32", ReadDefault: 20 } );
   schema.dropColumn( "b" );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult2 );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );
}