/******************************************************************************
 * @Description   : seqDB-31570 ::  创建全文索引指定映射为text类型，插入索引字段类型为兼容转换类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.16
 * @LastEditTime  : 2023.05.16
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31570";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   var doc = [{ no: 1, tlong: 3000000000, tstr: "2041-03-04", tdouble: -1.7E+308, tbool: true, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" }, tarr: ["test1", "test2"] },
   { no: -2147483648, tlong: { "$numberLong": "-9223372036854775808" }, tstr: "test02", tdouble: 1.7E+308, tbool: false, tdate: { "$date": "0000-01-01" }, ttime: { "$timestamp": "1902-01-01-00.00.00.000000" }, tarr: ["str@1", "str@2"] },
   { no: 2147483647, tlong: { "$numberLong": "9223372036854775807" }, tstr: "test01", tdouble: -0.123, tbool: false, tdate: { "$date": "9999-12-31" }, ttime: { "$timestamp": "2037-12-31-23.59.59.999999" }, tarr: ["arr!0", "atest02"] },
   { no: 2147483646, tlong: 2300000000, tstr: "2020-01-03T02:00:00", tdouble: 234.56, tbool: true, tdate: { "$date": "1970-01-01" }, ttime: { "$timestamp": "2022-01-01-03.14.26.124233" }, tarr: ["011", "234"] },
   { no: -1, tlong: { "$numberLong": "-1" }, tstr: "测试数据类型01", tdouble: 5000.46, tbool: false, tdate: { "$date": "2022-05-01" }, ttime: { "$timestamp": "2025-04-01-15.14.26.124233" }, tarr: ["test01", "atest02"] }];
   dbcl.insert( doc );

   var indexName = "index_31570";
   var mappingsOption =
   {
      "Fields": {
         "no": { "Type": "text" }, "tlong": { "Type": "text" }, "tstr": { "Type": "text" },
         "tdouble": { "Type": "text" }, "tbool": { "Type": "text" }, "tdate": { "Type": "text" }, "ttime": { "Type": "text" },
         "tarr": { "Type": "text" }
      }
   };
   dbcl.createIndex( indexName, { "no": "text", tlong: "text", tstr: "text", tdouble: "text", tbool: "text", tdate: "text", ttime: "text", tarr: "text" }, { "Mappings": mappingsOption } );

   snapshotIndexCheckMappings( dbcl, indexName, mappingsOption );
   listIndexCheckMappings( dbcl, indexName, mappingsOption );

   //全文检索数据
   var indexRecordNum = 5;
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );


   //检索string类型数据匹配“test”前缀的数据  
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "prefix": { "tstr": { "value": "test" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { tstr: { $regex: "test*", $options: "i" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   //检索int类型数据  
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "no": 2147483647 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "no": 2147483647 }, { _id: { "$include": 0 } }, { _id: 1 } );;
   checkResult( expectRecords3, actRecords3 );

   //检索long类型数据
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tlong": { "$numberLong": "9223372036854775807" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "tlong": { "$numberLong": "9223372036854775807" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );

   //检索double类型数据,映射为text类型，使用字符串方式检索模糊匹配
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tdouble": "1.7E+308" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords5 = dbOpr.findFromCL( dbcl, { "$or": [{ "tdouble": 1.7e+308 }, { "tdouble": -1.7e+308 }] }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords5, actRecords5 );

   //检索bool类型数据
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tbool": true } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords6 = dbOpr.findFromCL( dbcl, { "tbool": true }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords6, actRecords6 );

   //检索date类型数据
   var actRecords7 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tdate": "9999-12-31" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords7 = dbOpr.findFromCL( dbcl, { "tdate": { "$date": "9999-12-31" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords7, actRecords7 );

   //检索time类型数据
   var actRecords8 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "bool": { "filter": { "range": { "ttime": { "gte": "2035-04-02T03:14:26.124233", "lte": "2038-01-01T12:59:59.999999" } } } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords8 = dbOpr.findFromCL( dbcl, { "ttime": { "$gte": { "$timestamp": "2037-12-31-23.59.59.999999" } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords8, actRecords8 );

   //检索array类型数据
   var actRecords9 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tarr": "atest02" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords9 = dbOpr.findFromCL( dbcl, { "tarr": { "$elemMatch": { "$et": "atest02" } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords9, actRecords9 );

   //再次插入索引数据，检索新插入的数据
   var newRecords = [{ no: 2147483646, tlong: { "$numberLong": "9223372036854775800" }, tstr: "testnew", tdouble: -0.123, tbool: false, tdate: { "$date": "1901-12-31" }, ttime: { "$timestamp": "2007-12-31-23.59.59.999999" }, tarr: ["arr!0", "atest02"] }];
   dbcl.insert( newRecords );
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum + 1, esIndexNames );
   var actRecords10 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords10 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords10, actRecords10 );

   // 删除索引
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );
}

