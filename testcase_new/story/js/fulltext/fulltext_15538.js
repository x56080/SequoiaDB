/******************************************************************************
 * @Description   : seqDB-15538:多键全文索引，插入全文索引字段为数组类型的记录，全量同步/增量同步到ES
 * @Author        : liuxiaoxuan 
 * @CreateTime    : 2018.10.10
 * @LastEditTime  : 2023.05.13
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_15538";
main( test );

function test ()
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引前插入数据
   var doc = [{ no: 1, a: ["arr1"], b: ["arr1", "arr2", "arr3"], c: "abc", d: [12, 3] },
   { no: 2, a: ["arr1", 1], b: ["arr2", 1.001], c: ["arr3", 3000000000], d: ["arr4", { $decimal: "123.456" }] },
   { no: 3, a: ["arr1", { "$oid": "123abcd00ef12358902300ef" }], b: ["arr2", true], c: ["arr3", { "$date": "2019-09-01" }], d: ["arr4", { "$timestamp": "2019-09-01-13.14.26.124233" }] },
   { no: 4, a: ["arr1", { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }], b: ["arr2", { "$regex": "^abc", "$options": "i" }], c: ["arr3", { "subobj": "value" }] },
   { no: 5, a: ["arr1", null], b: ["arr2", { "$minKey": 1 }], c: ["arr3", { "$maxKey": 1 }] },
   { no: 6, a: [2, 2.222, -3000000000] },
   { no: 7, a: [{ $decimal: "-123.456" }, { "$oid": "123abcd00ef1235890230011" }, { "$date": "2019-10-01" }, { "$timestamp": "2019-10-01-13.14.26.124233" }] },
   { no: 8, a: [{ "$binary": "xQ12", "$type": "1" }, { "$regex": "^xyz", "$options": "i" }, { "x": "y" }] },
   { no: 9, a: [false, { "$minKey": 1 }, { "$maxKey": 1 }] },
   { no: 10, a: ["brr1"], b: "abc" },
   { no: 11, a: ["brr1"], b: [1, 1.001, 6000000000], c: "abc" },
   { no: 12, a: ["brr1", 1], b: ["brr2", "brr3", "brr4"], c: "bcd" },
   { no: 13, a: [true], b: [null], c: "efg" },
   { no: 14, a: [{ "$oid": "123abcd00ef1235890233456" }, { "$regex": "^opq", "$options": "i" }], b: [{ "$date": "2019-10-01" }, { "$timestamp": "2019-10-01-13.14.26.124233" }], c: "hij" },
   { no: 15, a: ["brr1"], b: 3, c: 3.003, d: 6000000000 },
   { no: 16, a: ["brr1", "brr2"], b: { $decimal: "567.089" }, c: { "$oid": "123abcd00ef1235890233456" }, d: { "$regex": "^opq", "$options": "i" } },
   { no: 17, a: ["brr1", "brr2", -1, -3.003], b: { "$date": "2019-10-01" }, c: { "$timestamp": "2019-10-01-13.14.26.124233" } },
   { no: 18, a: ["brr1", "brr2", true, { "$regex": "^opq", "$options": "i" }], b: true, c: null, d: { "k": "v" } },
   { no: 19, a: [-1, { "k": "v" }, { "$oid": "123abcd00ef1235890233456" }], b: { "$minKey": 1 }, c: { "$maxKey": 1 } },
   { no: 20, a: "opt", b: 5, c: 5.005, d: 5000000000 },
   { no: 21, a: "opt", b: { $decimal: "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } },
   { no: 22, a: "opt", b: { "$date": "2019-11-01" }, c: { "$timestamp": "2019-11-01-13.14.26.124233" }, d: null },
   { no: 23, a: "opt", b: { "$regex": "^abc", "$options": "i" }, c: { "$minKey": 1 }, d: { "$maxKey": 1 } },
   { no: 24, a: ["crr1"], b: ["crr1", "crr2", "crr2"], c: "ko", d: 6 },
   { no: 25, a: ["crr1", { "$oid": "123abcd00ef1235890238901" }], b: "kp", c: 6.006, d: { $decimal: "-555.666" } },
   { no: 26, a: [5, 5.555, -5000000000], b: ["crr2 fail,crr3"], c: "kq", d: -5000000000 },
   { no: 27, a: [{ "$binary": "qe91", "$type": "1" }, { "$regex": "^zzz", "$options": "i" }, { "a": "b" }, "abc"], b: "kx", c: { "a": "b" }, d: null }
   ];

   dbcl.insert( doc );

   var textIndexName = "textIndex_15538";
   dbcl.createIndex( textIndexName, { "a": "text", "b": "text", "c": "text", "d": "text" } );
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );

   // 检查ES数组是否同步
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 7, esIndexNames );

   // 检查全文检索结果，match_all匹配
   var dbOpr = new DBOperator();
   var expectResult = [{ no: 10, a: ["brr1"], b: "abc" },
   { no: 15, a: ["brr1"], b: 3, c: 3.003, d: 6000000000 },
   { no: 16, a: ["brr1", "brr2"], b: { $decimal: "567.089" }, c: { "$oid": "123abcd00ef1235890233456" }, d: { "$regex": "^opq", "$options": "i" } },
   { no: 20, a: 'opt', b: 5, c: 5.005, d: 5000000000 },
   { no: 21, a: 'opt', b: { "$decimal": "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } },
   { no: 23, a: "opt", b: { "$regex": "^abc", "$options": "i" }, c: { "$minKey": 1 }, d: { "$maxKey": 1 } },
   { no: 25, a: ["crr1", { "$oid": "123abcd00ef1235890238901" }], b: "kp", c: 6.006, d: { $decimal: "-555.666" } }
   ];
   var actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectResult, actResult );

   // 创建索引后，插入记录多于1个数组元素，插入报错
   assert.tryThrow( SDB_IXM_MULTIPLE_ARRAY, function()
   {
      dbcl.insert( doc );
   } );

   // 创建全文索引后正常插入数据
   doc = [{ no: 26, a: [2, 2.222, -3000000000] },
   { no: 27, a: [{ $decimal: "-123.456" }, { "$oid": "123abcd00ef1235890230011" }, { "$date": "2019-10-01" }, { "$timestamp": "2019-10-01-13.14.26.124233" }] },
   { no: 28, a: "opt", b: 5, c: 5.005, d: 5000000000 },
   { no: 29, a: "opt", b: { $decimal: "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } },
   { no: 30, a: "234.5", b: [23, 67], c: "testc", d: "234.56" }
   ];

   dbcl.insert( doc );

   // 检查ES数组是否同步
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 9 );

   // 检查全文检索结果，match_all匹配
   actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   expectResult = [{ no: 10, a: ["brr1"], b: "abc" },
   { no: 15, a: ["brr1"], b: 3, c: 3.003, d: 6000000000 },
   { no: 16, a: ["brr1", "brr2"], b: { $decimal: "567.089" }, c: { "$oid": "123abcd00ef1235890233456" }, d: { "$regex": "^opq", "$options": "i" } },
   { no: 20, a: 'opt', b: 5, c: 5.005, d: 5000000000 },
   { no: 21, a: 'opt', b: { "$decimal": "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } },
   { no: 23, a: "opt", b: { "$regex": "^abc", "$options": "i" }, c: { "$minKey": 1 }, d: { "$maxKey": 1 } },
   { no: 25, a: ["crr1", { "$oid": "123abcd00ef1235890238901" }], b: "kp", c: 6.006, d: { $decimal: "-555.666" } },
   { no: 28, a: "opt", b: 5, c: 5.005, d: 5000000000 },
   { no: 29, a: "opt", b: { $decimal: "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } }
   ];
   checkResult( expectResult, actResult );

   // 检查全文检索结果，match匹配
   actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "opt" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   expectResult = [{ no: 20, a: 'opt', b: 5, c: 5.005, d: 5000000000 },
   { no: 21, a: 'opt', b: { "$decimal": "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } },
   { no: 23, a: "opt", b: { "$regex": "^abc", "$options": "i" }, c: { "$minKey": 1 }, d: { "$maxKey": 1 } },
   { no: 28, a: "opt", b: 5, c: 5.005, d: 5000000000 },
   { no: 29, a: "opt", b: { $decimal: "111.222" }, c: { "$oid": "123abcd00ef1235890238901" }, d: { "$binary": "re81", "$type": "1" } }
   ];
   checkResult( expectResult, actResult );

   // 删除集合中的所有记录，全文检索返回0条
   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );
   actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   expectResult = [];
   checkResult( expectResult, actResult );
}
