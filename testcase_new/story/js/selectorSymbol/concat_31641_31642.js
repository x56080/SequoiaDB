/******************************************************************************
 * @Description   : seqDB-31641:$concat 方法二基本功能验证
 *                  seqDB-31642:$concat 数组包含不能处理的元素
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.18
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31641_31642";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var docs = [
      { No: 1, a: "sdb" },
      { No: 2, a: ["sdb", "123"] },
      { No: 3, a: ["abc", " "] },
      { No: 4, a: ["abc", null] },
      { No: 5, a: ["abc", "\\\""] }
   ];
   dbcl.insert( docs );

   // { <字段名> : { $concat : [ pos, [ "<字符串>", ... ] ] } }
   // 作为选择符
   // pos为0
   var actRecords = dbcl.find( {}, { a: { "$concat": [0, ["aaa", "bbb", "ccc"]] } } );
   var expRecords = [
      { No: 1, a: "sdbaaabbbccc" },
      { No: 2, a: ["sdbaaabbbccc", "123aaabbbccc"] },
      { No: 3, a: ["abcaaabbbccc", " aaabbbccc"] },
      { No: 4, a: ["abcaaabbbccc", null] },
      { No: 5, a: ["abcaaabbbccc", "\\\"aaabbbccc"] }
   ];
   commCompareResults( actRecords, expRecords );

   // pos为1
   actRecords = dbcl.find( {}, { a: { "$concat": [1, ["aaa", "bbb", "ccc"]] } } );
   expRecords = [
      { No: 1, a: "aaasdbbbbccc" },
      { No: 2, a: ["aaasdbbbbccc", "aaa123bbbccc"] },
      { No: 3, a: ["aaaabcbbbccc", "aaa bbbccc"] },
      { No: 4, a: ["aaaabcbbbccc", null] },
      { No: 5, a: ["aaaabcbbbccc", "aaa\\\"bbbccc"] }
   ];
   commCompareResults( actRecords, expRecords );

   // pos大于数组长度
   actRecords = dbcl.find( {}, { a: { "$concat": [3, ["aaa", "bbb", "ccc"]] } } );
   expRecords = [
      { No: 1, a: "aaabbbcccsdb" },
      { No: 2, a: ["aaabbbcccsdb", "aaabbbccc123"] },
      { No: 3, a: ["aaabbbcccabc", "aaabbbccc "] },
      { No: 4, a: ["aaabbbcccabc", null] },
      { No: 5, a: ["aaabbbcccabc", "aaabbbccc\\\""] }
   ];
   commCompareResults( actRecords, expRecords );

   // pos为-1
   actRecords = dbcl.find( {}, { a: { "$concat": [-1, ["aaa", "bbb", "ccc"]] } } );
   expRecords = [
      { No: 1, a: "aaabbbcccsdb" },
      { No: 2, a: ["aaabbbcccsdb", "aaabbbccc123"] },
      { No: 3, a: ["aaabbbcccabc", "aaabbbccc "] },
      { No: 4, a: ["aaabbbcccabc", null] },
      { No: 5, a: ["aaabbbcccabc", "aaabbbccc\\\""] }
   ];
   commCompareResults( actRecords, expRecords );

   // pos<0且绝对值大于数组长度
   actRecords = dbcl.find( {}, { a: { "$concat": [-4, ["aaa", "bbb", "ccc"]] } } );
   expRecords = [
      { No: 1, a: "sdbaaabbbccc" },
      { No: 2, a: ["sdbaaabbbccc", "123aaabbbccc"] },
      { No: 3, a: ["abcaaabbbccc", " aaabbbccc"] },
      { No: 4, a: ["abcaaabbbccc", null] },
      { No: 5, a: ["abcaaabbbccc", "\\\"aaabbbccc"] }
   ];
   commCompareResults( actRecords, expRecords );

   // 作为匹配符
   // pos为0
   actRecords = dbcl.find( { a: { "$concat": [0, ["aaa", "bbb", "ccc"]], $et: "sdbaaabbbccc" } } );
   expRecords = [{ No: 1, a: "sdb" }, { No: 2, a: ["sdb", "123"] }];
   commCompareResults( actRecords, expRecords );

   // pos为1
   actRecords = dbcl.find( { a: { "$concat": [1, ["aaa", "bbb", "ccc"]], $et: "aaasdbbbbccc" } } );
   commCompareResults( actRecords, expRecords );

   // pos大于数组长度
   actRecords = dbcl.find( { a: { "$concat": [3, ["aaa", "bbb", "ccc"]], $et: "aaabbbcccsdb" } } );
   commCompareResults( actRecords, expRecords );

   // pos为-1
   actRecords = dbcl.find( { a: { "$concat": [-1, ["aaa", "bbb", "ccc"]], $et: "aaabbbcccsdb" } } );
   commCompareResults( actRecords, expRecords );

   // pos<0且绝对值大于数组长度
   actRecords = dbcl.find( { a: { "$concat": [-4, ["aaa", "bbb", "ccc"]], $et: "sdbaaabbbccc" } } );
   commCompareResults( actRecords, expRecords );
}