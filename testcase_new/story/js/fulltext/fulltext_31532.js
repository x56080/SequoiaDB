/******************************************************************************
 * @Description   : seqDB-31532 :: 使用通配符全文检索
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.23
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31532";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var indexName = "index_31526";
   dbcl.createIndex( indexName, {
      "no": "text", tstr: "text", tobj: "text", tarr: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "long" }, "tstr": { "Type": "text" }, "tobj.key": { "Type": "text" }, "tarr": { "Type": "keyword" }
         }
      }
   } );

   var doc = [{ no: 1, tstr: "2041-03-04", tobj: { "key": "valuetam" }, tarr: ["测试数据", "test2"] },
   { no: -2147483648, tstr: "test02", tobj: { "key": "valueiam" }, tarr: ["str@1", "str@2"] },
   { no: 2147483647, tstr: "2041-03-01", tobj: { "key": "Hello world!" }, tarr: ["arr!0", "atest"] },
   { no: { "$numberLong": "-9223372036854775808" }, tstr: "Hello World", tobj: { "key": "value" }, tarr: ["011", "234"] },
   { no: -1, tstr: "测试数据类型01", tobj: { "key": "2031-06-05" }, tarr: ["test01", "测试数据test"] },
   { no: 3000000000, tstr: "hello World ", tobj: { "key": "test value" }, tarr: ["test01", "atest02"] },
   { no: 7, tstr: "hello world xx!", tobj: { "key": "测试数据类型" }, tarr: ["test01", "atest02"] }];
   dbcl.insert( doc );


   //全文检索数据
   var indexRecordNum = 7;
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
   //test a:使用通配符*全文检索
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "wildcard": { "tstr": "hello*" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, { "tstr": { $regex: "hello*", $options: "i" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "wildcard": { "tarr": "测试*" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "tarr": { $regex: "测试*", $options: "i" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );


   //test b:使用通配符？全文检索
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "wildcard": { "tobj.key": "value?am" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = [{ no: 1, tstr: "2041-03-04", tobj: { "key": "valuetam" }, tarr: ["测试数据", "test2"] },
   { no: -2147483648, tstr: "test02", tobj: { "key": "valueiam" }, tarr: ["str@1", "str@2"] }]
   checkResult( expectRecords3, actRecords3 );

   //test c:使用通配符*和？全文检索
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "query_string": { "query": "20?1*", 'fields': ["tstr", "tobj.key"] } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = [{ "no": 1, "tstr": "2041-03-04", "tobj": { "key": "valuetam" }, "tarr": ["测试数据", "test2"] }, { "no": 2147483647, "tstr": "2041-03-01", "tobj": { "key": "Hello world!" }, "tarr": ["arr!0", "atest"] }, { "no": -1, "tstr": "测试数据类型01", "tobj": { "key": "2031-06-05" }, "tarr": ["test01", "测试数据test"] }];
   checkResult( expectRecords4, actRecords4 );

}


