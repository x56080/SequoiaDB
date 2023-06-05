/******************************************************************************
 * @Description   : seqDB-31526 :: 指定条件精确查询
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.23
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31526";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   var doc = [{ no: 1, tstr: "2041-03-04", tdouble: -1.7E+308, tdate: { "$date": "2012-01-01" }, tarr: ["测试数据", "test2"] },
   { no: -2147483648, tstr: "test02", tdouble: 1.7E+308, tdate: { "$timestamp": "2002-01-01-13.14.26.124233" }, tarr: ["str@1", "str@2"] },
   { no: 2147483647, tstr: "test01", tdouble: -0.123, tdate: { "$date": "9999-12-31" }, tarr: ["arr!0", "atest"] },
   { no: { "$numberLong": "-9223372036854775808" }, tstr: "Hello World", tdouble: 234.56, tdate: { "$date": "1970-01-01" }, tarr: ["011", "234"] },
   { no: -1, tstr: "测试数据类型01", tdouble: 5000.46, tdate: { "$date": "2022-05-01" }, tarr: ["test01", "测试数据test"] },
   { no: 3000000000, tstr: "Hello World xx!", tdouble: 340.46, tdate: { "$date": "2032-05-01" }, tarr: ["test01", "atest02"] },
   { no: 7, tstr: "Hello world xx!", tdouble: -123.35, tdate: { "$date": "2042-12-01" }, tarr: ["test01", "atest02"] }];
   dbcl.insert( doc );

   var indexName = "index_31526";
   dbcl.createIndex( indexName, {
      "no": "text", tstr: "text", tdouble: "text", tdate: "text", tarr: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "long" }, "tstr": { "Type": "keyword" }, "tdouble": { "Type": "double" }, "tdate": { "Type": "date" },
            "tarr": { "Type": "keyword" }
         }
      }
   } );

   //全文检索数据
   var indexRecordNum = 7;
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
   //test a:完全匹配检索
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tstr": "Hello World" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, { "tstr": "Hello World" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   //test b:terms通过数组指定搜索多个完整词汇
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "terms": { "tarr": ["测试数据", "test01"] } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "$or": [{ "tarr": { $regex: "test01*", $options: "i" } }, { "tarr": { $regex: "测试数据", $options: "i" } }] }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   //test c:range范围查询，搜索数值范围:double
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "range": { "tdouble": { "gte": 0, "lte": 5000.46 } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "tdouble": { "$gte": 0, "$lte": 5000.46 } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords3, actRecords3 );

   //test c:range范围查询，搜索数值范围:long
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "range": { "no": { "gte": -2147483648, "lte": 2147483647 } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "no": { "$gte": -2147483648, "$lte": 2147483647 } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );

   //test d:range范围查询，搜索日期范围
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "range": { "tdate": { "gte": "1970-01-01", "lte": "2022-05-01" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "tdate": { "$gte": { "$date": "1970-01-01" }, "$lte": { "$date": "2022-05-01" } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );
}


