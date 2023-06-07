/******************************************************************************
 * @Description   : seqDB-31511:创建全文索引，索引字段为数组类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31511";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 插入数据
   var docs = [
      { no: 1, tArray: ["test", 123, { "$numberLong": "123" }] },
      { no: 2, tArray: ["test", { "$date": "2012-04-05" }, { "city": "guangzhou" }] },
      { no: 3, tArray: [false, true] },
      { no: 4, tArray: ["test1", "字符串", "city"] },
      { no: 5, tArray: ["123", "456"] },
      { no: 6, tArray: [{ "$numberLong": "2147483647" }, { "$numberLong": "1000000" }] },
      { no: 7, tArray: [123.456, 12.12, 0.1] },
      { no: 8, tArray: [{ "$date": "2012-01-01" }, { "$date": "0000-01-01" }, { "$date": "9999-12-31" }] },
      { no: 9, tArray: [{ "$timestamp": "2012-01-01-13.14.26.124233" }, { "$timestamp": "2000-01-01-13.14.26.124233" }, { "$timestamp": "1999-12-31-13.14.26.124233" }] },
      { no: 10, tArray: [{ "city": "guangzhou" }, { "city": "shanghai" }] },
      { no: 11, tArray: [["test1"], ["test2", "tets3"]] },
      { no: 12, tArray: ["test1", ["test2", "test3"]] },
      { no: 12, tArray: ["test", [123, { "$numberLong": "123" }], 12.3] },
      { no: 13, tArray: ["test", ["123", "12.3"], "2020-01-03"] }
   ];
   dbcl.insert( docs );

   // 创建全文索引，索引字段为数组类型
   var idxName = "idx_31511";
   dbcl.createIndex( idxName, { "tArray": "text" }, { "Mappings": { "Fields": { "tArray": { "Type": "text" } } } } );

   // 匹配字段tArray，检查全文索引结果
   var indexRecordNum = 8;
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [
      { no: 3, tArray: [false, true] },
      { no: 4, tArray: ["test1", "字符串", "city"] },
      { no: 5, tArray: ["123", "456"] },
      { no: 6, tArray: [2147483647, 1000000] },
      { no: 7, tArray: [123.456, 12.12, 0.1] },
      { no: 8, tArray: [{ "$date": "2012-01-01" }, { "$date": "0000-01-01" }, { "$date": "9999-12-31" }] },
      { no: 9, tArray: [{ "$timestamp": "2012-01-01-13.14.26.124233" }, { "$timestamp": "2000-01-01-13.14.26.124233" }, { "$timestamp": "1999-12-31-13.14.26.124233" }] },
      { no: 11, tArray: [["test1"], ["test2", "tets3"]] },
   ];
   checkResult( expectRecords1, actRecords1 );

   // 匹配数组元素，检查全文索引结果
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tArray": "test1" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = [
      { no: 4, tArray: ["test1", "字符串", "city"] },
      { no: 11, tArray: [["test1"], ["test2", "tets3"]] }
   ];
   checkResult( expectRecords2, actRecords2 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}