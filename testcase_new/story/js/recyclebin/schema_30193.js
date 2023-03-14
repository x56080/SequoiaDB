/******************************************************************************
 * @Description   : seqDB-30193:集合绑定外部模式，回收站恢复truncate项目
 * @Author        : liuli
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30193";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
testConf.clName = COMMCLNAME + "_30193";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcl = args.testCL;

   // 绑定外部模式
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
      expPrimalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 集合执行truncate后恢复truncate项目
   dbcl.truncate();
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, clName, testConf.schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 再次插入数据
   docs = [];
   for( var i = 1000; i < 2000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
      expPrimalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 校验内部模式
   var expInternalColumnDef = { "a": {}, "b": { ReadDefault: 10 } };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, clName, testConf.schemaName );
}