/******************************************************************************
 * @Description   : seqDB-31534:创建索引，同时指定普通索引字段和全文索引字段，插入/更新/删除/查询索引数据
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31534";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var dbOpr = new DBOperator();

   // 创建全文索引
   var idxName = "index_31534_text";
   dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Type": "text" } } } } );

   // 插入数据
   var docs = [
      { no: 1, tlong: 3000000000, tstr: "2041-03-04", tdouble: -1.7E+308, tbool: true, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" }, tarr: ["test1", "test2"], tobj: { "city": "shanghai" } },
      { no: 66, tlong: { "$numberLong": "-9223372036854775808" }, tstr: "2000-01-01", tdouble: 1.7E+308, tbool: false, tdate: { "$date": "0000-01-01" }, ttime: { "$timestamp": "2000-01-01-00.00.00.000000" }, tarr: ["str@1", "str@2"], tobj: { "city": "beijing" } },
      { no: 100, tlong: { "$numberLong": "9223372036854775807" }, tstr: "test01", tdouble: -0.123, tbool: false, tdate: { "$date": "9999-12-31" }, ttime: { "$timestamp": "2037-12-31-23.59.59.999999" }, tarr: ["arr!0", "atest02"], tobj: { "city": "guangzhou" } },
      { no: 166, tlong: 2300000000, tstr: "2020-01-03T02:00:00", tdouble: 234.56, tbool: true, tdate: { "$date": "1970-01-01" }, ttime: { "$timestamp": "2022-01-01-03.14.26.124233" }, tarr: ["011", "234"], tobj: { "city": "shengzhen" } },
      { no: 188, tlong: { "$numberLong": "-1" }, tstr: "测试数据类型01", tdouble: 5000.46, tbool: false, tdate: { "$date": "2022-05-01" }, ttime: { "$timestamp": "2025-04-01-15.14.26.124233" }, tarr: ["test01", "atest02"], tobj: { "city": "wuhan" } }
   ];
   dbcl.insert( docs );

   // 创建普通索引
   var indexName_1 = "index_31534_normal";
   dbcl.createIndex( indexName_1, { tlong: 1 } );

   var actResult = dbcl.find( { tlong: 3000000000 } ).explain().current().toObj()["IndexName"];
   checkResult( indexName_1, actResult );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "tlong": 3000000000 }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords1 = [{ no: 1, tlong: 3000000000, tstr: "2041-03-04", tdouble: -1.7E+308, tbool: true, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" }, tarr: ["test1", "test2"], tobj: { "city": "shanghai" } }];
   checkResult( expRecords1, actRecords1 );

   // 检查全文索引结果
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "2041-03-04" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords1, actRecords2 );

   // 更新记录
   dbcl.update( { "$set": { "tlong": 1, "tstr": "newString" } }, { "tlong": 3000000000 } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecord3 = dbOpr.findFromCL( dbcl, { "tlong": 1 }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecord3 = [{ no: 1, tlong: 1, tstr: "newString", tdouble: -1.7E+308, tbool: true, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" }, tarr: ["test1", "test2"], tobj: { "city": "shanghai" } }];
   checkResult( expRecord3, actRecord3 );

   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "newString" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecord3, actRecords4 );

   // 删除记录
   dbcl.remove( { tlong: 1 } );
   var actRecords5 = dbOpr.findFromCL( dbcl, { "tlong": 1 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [], actRecords5 );

   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum - 1, esIndexNames );
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tlong": 1 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [], actRecords6 );
}