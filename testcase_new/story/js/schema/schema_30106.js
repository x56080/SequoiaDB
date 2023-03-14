/******************************************************************************
 * @Description   : seqDB-30106:挂载存在索引的表，索引和外部模式冲突
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var mainCLName = "mainCL_30106";
   var subCLName1 = "subCL_30106_1";
   var subCLName2 = "subCL_30106_2";
   var schemaName = "schema_30106";
   var indexName = "index_30106";
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName );

   commCreateSchema( db, schemaName, schemaDef );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   var subcl = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   // 主表绑定外部模式
   maincl.addSchema( schemaName );

   // 子表创建索引，索引字段为外部模式中设置读默认值的字段
   subcl.createIndex( indexName, { b: 1 } );
   var docs = [];
   for( var i = 1000; i < 1010; i++ )
   {
      docs.push( { a: i } );
   }
   subcl.insert( docs );

   // 挂载子表
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   } );

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName );
}