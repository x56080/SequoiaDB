/******************************************************************************
 * @Description   : seqDB-31636:$concat函数参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.17
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31636";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   dbcl.insert( { No: 1, a: "abc" } );
   // { <字段名> : { $concat : "<字符串>" } }
   // <字符串>为数组
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      actRecords = dbcl.find( {}, { "a": { "$concat": [1, 2, 3] } } ).toArray();
   } );

   // <字符串>为null
   var actRecords = dbcl.find( {}, { "a": { "$concat": null } } );
   var expRecords = [{ No: 1, a: null }];
   commCompareResults( actRecords, expRecords );

   // <字符串>为BinData
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } } );
   commCompareResults( actRecords, expRecords );

   // <字符串>为RegEx
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$regex": "^W", "$options": "i" } } } );
   commCompareResults( actRecords, expRecords );

   // <字符串>为MinKey
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$minKey": 1 } } } );
   commCompareResults( actRecords, expRecords );

   // <字符串>为MaxKey
   actRecords = dbcl.find( {}, { "a": { "$concat": { "$maxKey": 1 } } } );
   commCompareResults( actRecords, expRecords );

   // 空串调用该函数
   dbcl.insert( { No: 2, a: "" } );
   actRecords = dbcl.find( {}, { "a": { "$concat": "abc" } } );
   expRecords = [{ No: 1, a: "abcabc" }, { No: 2, a: "abc" }];
   commCompareResults( actRecords, expRecords );
}