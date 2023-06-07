/******************************************************************************
 * @Description   : seqDB-31541:带全文索引条件更新记录
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31541";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引指定映射为integer类型
   var idxName = "idx_31541";
   dbcl.createIndex( idxName, {
      no: "text", tstr: "text", tlong: "text", tdouble: "text", tarr: "text"
   }, { "Mappings": { "Fields": { "no": { "Type": "integer" } } } } );

   // 插入数据
   var docs = [
      { no: 1, tstr: "123", tlong: { "$numberLong": "123" }, tdouble: 123.456, tarr: [123, 456, 789] },
      { no: 2147483647, tstr: "2147483647", tlong: { "$numberLong": "2147483647" }, tarr: [2147483647, 0] },
      { no: -2147483648, tstr: "-2147483648", tlong: { "$numberLong": "-2147483648" }, tarr: [-12147483648, 0] },
      { no: -1, tstr: "-1", tlong: { "$numberLong": "2147483647" }, tarr: [-1, 1, 0, 2] }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 4;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );

   // 带全文索引条件更新记录，检查结果
   dbcl.update( { "$set": { "no": 2 } }, { "no": 1 } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "no": 1 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [], actRecords2 );
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "no": 2 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "no": 2 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords3, actRecords3 );
}