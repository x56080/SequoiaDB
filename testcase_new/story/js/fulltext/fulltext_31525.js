/******************************************************************************
 * @Description   : seqDB-31525:创建全文索引，更新索引字段值（不兼容映射类型）
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31525";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31525";
   dbcl.createIndex( idxName, { "tstr": "text" }, { "Mappings": { "Fields": { "tstr": { "Type": "text" } } } } );

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

   // 更新索引字段为不兼容类型
   dbcl.update( { "$set": { "tstr": { "city": "beijing" } } }, { "tstr": "123" } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr.city": "beijing" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords2 = [];
   checkResult( expRecords2, actRecords2 );
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "abc" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords3 = dbOpr.findFromCL( dbcl, { "tstr": "abc" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords3, actRecords3 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}