/******************************************************************************
 * @Description   : seqDB-30351:重命名恢复truncate项目
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_30351";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( args )
{
   cleanRecycleBin( db, COMMCSNAME );
   var newCL = "newcl_30351";
   var originName = COMMCSNAME + "." + testConf.clName;
   var newName = COMMCSNAME + "." + newCL;
   commDropCL( db, COMMCSNAME, newCL );
   var schemaName1 = "schema_30351_1";
   var schemaName2 = "schema_30351_2";
   var schemaName3 = "schema_30351_3";
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );
   commDropSchema( db, schemaName3 );

   var dbcl = args.testCL;

   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 10, WriteDefault: 20 },
      "b": { Type: "int32", ReadDefault: 30, WriteDefault: 40 }
   };
   commCreateSchema( db, schemaName1, schemaDef );
   // 绑定外部模式
   dbcl.addSchema( schemaName1 );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 40 } );
      expPrimalResult.push( { a: i, b: 40 } );
   }
   dbcl.insert( docs );

   // 集合执行truncate,重命名恢复回收站truncate项目
   dbcl.truncate();
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   db.getRecycleBin().returnItemToName( recycleName, newName );

   // println( dbcl.getInternalSchema() );
   // checkAddSchema( db, COMMCSNAME, testConf.clName, schemaName1 );
   // println( db.snapshot( SDB_SNAP_CATALOG, { "Name": COMMCSNAME + "." + testConf.clName, "Schema": schemaName1 } ) );
   // println( db.snapshot( SDB_SNAP_CATALOG, { "Name": COMMCSNAME + "." + newCL, "Schema": schemaName1 } ) );

   // 贴源、非贴源校验数据
   var newcl = db.getCS( COMMCSNAME ).getCL( newCL );
   var actResult = newcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = newcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   var schemaDef2 = {
      "a": { Type: "int32", ReadDefault: 100, WriteDefault: 200 },
      "b": { Type: "int32", ReadDefault: 300, WriteDefault: 400 }
   };
   commCreateSchema( db, schemaName2, schemaDef2 );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      newcl.addSchema( schemaName2 );
   } )

   // commCreateSchema( db, schemaName3, schemaDef );
   // newcl.addSchema( schemaName3 );

   // 再次插入数据
   docs = [];
   for( var i = 10; i < 20; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 40 } );
      expPrimalResult.push( { a: i, b: 40 } );
   }
   newcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = newcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = newcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 校验内部模式
   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 10,
         "WriteDefault": 20
      },
      "b": {
         "ReadDefault": 30,
         "WriteDefault": 40
      }
   };
   checkInternalSchema( newcl, expInternalColumnDef );

   // 校验外部模式和集合绑定关系
   // println( db.list( SDB_LIST_SCHEMAS ) );
   // TODO：已经重命名了，但是绑定schema的集合还是原来的名字？
   // checkAddSchema( db, COMMCSNAME, testConf.clName, schemaName1 );
   // checkAddSchema( db, COMMCSNAME, newCL, schemaName1 );
   commDropCL( db, COMMCSNAME, newCL );
}