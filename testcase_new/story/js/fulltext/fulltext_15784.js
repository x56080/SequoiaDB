/******************************************************************************
 * @Description   : seqDB-15784:多键全文索引，更新部分字段为数组，且数组元素部分为string
 * @Author        : liuxiaoxuan 
 * @CreateTime    : 2018.10.10
 * @LastEditTime  : 2023.05.12
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_15784";
main( test );

function test ()
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   var doc = [{ a: "aaa", b: "bbb", c: "ccc", no: 1 },
   { a: ["brr1"], b: "abc", c: "def", no: 2 },
   { a: ["brr1", "brr2", "brr3"], b: "bac", c: "fed", no: 3 },
   { a: ["arr1", 1], b: ["arr2", 1.001], c: ["arr3", 3000000000], d: "hjk", no: 4 },
   { a: ["arr1", { "$oid": "123abcd00ef12358902300ef" }], b: "lmn", c: "opt", no: 5 },
   { a: [{ 0: 1 }, { 1: 2 }, { 3: 4 }, 2, 2.222, -3000000000, { $decimal: "123.456" }], b: "rsv", c: "unw", no: 6 },
   { a: { "$oid": "123abcd00ef1235890233456" }, b: { "$date": "2019-10-01" }, c: { "$timestamp": "2019-10-01-13.14.26.124233" }, d: null, no: 7 },
   { a: { "$binary": "qe91", "$type": "1" }, b: { "$regex": "^zzz", "$options": "i" }, c: { "a": "b" }, no: 8 }
   ];
   dbcl.insert( doc );

   var textIndexName = "textIndex_15784";
   dbcl.createIndex( textIndexName, { "a": "text", "b": "text", "c": "text", "d": "text" } );

   // 更新后的记录字段部分为string类型
   dbcl.update( { $set: { a: ["update 1", 1], b: true, c: 0.1111, d: 9000000000 } } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 8 );

   // 检查全文检索结果
   var dbOpr = new DBOperator();
   var expResult = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   var actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expResult, actResult );
}
