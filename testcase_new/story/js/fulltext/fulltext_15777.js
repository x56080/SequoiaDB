/******************************************************************************
 * @Description   : seqDB-15777:单键全文索引，更新数组为非string
 * @Author        : liuxiaoxuan 
 * @CreateTime    : 2018.10.10
 * @LastEditTime  : 2023.05.11
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_15777";
main( test );

function test ()
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引前插入数据
   var doc = [{ a: ["arr1"] },
   { a: "arr1" },
   { a: ["arr1", "arr2", "arr3"] },
   { a: [{ "subobj": "value" }, { "k": "v" }, { "a": "b" }] },
   { a: [1, 1.001, 3000000000, { $decimal: "123.456" }] },
   { a: ["abc", { "o": "m" }, { "p": "q" }, "def"] },
   { a: ["hjk", { "o": 1 }, { "p": 3.2 }, { "q": 3000000000 }, { "r": { $decimal: "123.456" } }, { "s": null }, "def"] },
   { a: ["brr1", "123", { "a": "a" }, 1, 3.22] },
   { a: ["crr1", null, true] },
   { a: ["drr1", { "$oid": "123abcd00ef1235890233456" }, { "$regex": "^opq", "$options": "i" }, { $decimal: "567.089" }] },
   { a: ["err1", { "$date": "2019-10-01" }, { "$timestamp": "2019-10-01-13.14.26.124233" }] },
   { a: ["frr1", { "$minKey": 1 }, { "$maxKey": 1 }, { "$binary": "re81", "$type": "1" }] }
   ];
   dbcl.insert( doc );

   var textIndexName = "textIndex_15777";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // 更新至非string类型的记录
   dbcl.update( { "$set": { a: -1 } }, { a: { "$isnull": 0 } } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 12 );

   // 检查全文检索结果   
   var expResult = dbOpr.findFromCL( dbcl, {}, { "_id": { "$include": 0 } }, { _id: 1 } );
   var actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { "_id": { "$include": 0 } }, { _id: 1 } );
   checkResult( expResult, actResult );
}
