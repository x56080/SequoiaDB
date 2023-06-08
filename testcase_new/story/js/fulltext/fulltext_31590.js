/******************************************************************************
 * @Description   : seqDB-31590:创建全文索引，部分字段不建立映射关系
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.26
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31590";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31589";
   var mappingsInfo = { "Fields": { "tstr": { "Type": "keyword" }, "tdate": { "Index": false } } };
   dbcl.createIndex( idxName, { "tstr": "text", "tdate": "text", "tint": "text", "tobj": "text" }, { "Mappings": mappingsInfo } );
   listIndexCheckMappings( dbcl, idxName, mappingsInfo );
   snapshotIndexCheckMappings( dbcl, idxName, mappingsInfo );
   
   // 插入数据
   var docs = [
      { tstr: "123", tdate: { "$date": "1970-01-01" }, tint: 2147483647, tobj: { "city": "guangzhou" } },
      { tstr: "abc", tdate: { "$date": "2020-12-12" }, tint: 0, tobj: { a: 1.001 } },
      { tstr: "字符串", tdate: { "$date": "2023-05-25" }, tint: -2147483648, tobj: { a: 1, b: 2 } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 3;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords1, actRecords1 );

   // 检查string类型字段
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "字符串" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "tstr": "字符串" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // 检查index为false的字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tdate": "2020-12-12" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );

   } );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}