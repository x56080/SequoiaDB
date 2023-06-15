/******************************************************************************
 * @Description   : seqDB-31548:$substrCP作为匹配符使用
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.09
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31548";

main( test )
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 集合插入数据
   var docs = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "  Sequoiadb" },
      { a: "Se%^&quoiadb" },
      { a: "巨杉sequoiadb" },
      { a: ["Sequoiadb", 111] },
      { a: 111 },
      { a: 2022.0211 },
      { a: true },
      { a: { $date: "2022-02-11T15:59:59.999Z" } },
      { a: { $timestamp: "2022-02-11T15:59:59.999Z" } },
      { a: "éè𐍈𝄞𠮷" },
      { a: ".?\\$`" }
   ];
   dbcl.insert( docs );

   // 1.{ $substrCP: <value> } 
   // value为正整数
   var actResult = dbcl.find( { a: { $substrCP: 3, $et: "Seq" } } );
   var expResult2 = [
      { a: "Sequoiadb" },
      { a: ["Sequoiadb", 111] }
   ];
   commCompareResults( actResult, expResult2 );

   // value为负整数
   actResult = dbcl.find( { a: { $substrCP: -3, $et: "adb" } } );
   var expResult5 = [
      { "a": "Sequoiadb" },
      { "a": "  Sequoiadb" },
      { "a": "Se%^&quoiadb" },
      { "a": "巨杉sequoiadb" },
      { "a": ["Sequoiadb", 111] }
   ]
   commCompareResults( actResult, expResult5 );

   // value为负整数
   actResult = dbcl.find( { a: { $substrCP: 0, $et: "" } } );
   expResult0 = [
      { "a": "Sequoiadb" },
      { "a": "se" },
      { "a": "" },
      { "a": "  Sequoiadb" },
      { "a": "Se%^&quoiadb" },
      { "a": "巨杉sequoiadb" },
      { "a": ["Sequoiadb", 111] },
      { "a": "éè𐍈𝄞𠮷" },
      { "a": ".?\\$`" }
   ]
   commCompareResults( actResult, expResult0 );

   // value大于字符串长度
   actResult = dbcl.find( { a: { $substrCP: 100, $et: "Sequoiadb" } } );
   commCompareResults( actResult, expResult2 );

   // { $substrCP: [<pos>, <len>] }
   // pos为正整数，len为正整数
   actResult = dbcl.find( { a: { $substrCP: [3, 3], $et: "$`" } } );
   commCompareResults( actResult, [{ a: ".?\\$`" }] );

   // pos为负整数，len为正整数
   actResult = dbcl.find( { a: { $substrCP: [-3, 3], $et: "adb" } } );
   commCompareResults( actResult, expResult5 );

   // pos为0，len为正整数
   actResult = dbcl.find( { a: { $substrCP: [0, 3], $et: "Se%" } } );
   commCompareResults( actResult, [{ a: "Se%^&quoiadb" }] );

   // pos大于字符串长度，len为正整数
   actResult = dbcl.find( { a: { $substrCP: [100, 3], $et: "" } } );
   commCompareResults( actResult, expResult0 );

   // pos为正整数，len为负整数
   actResult = dbcl.find( { a: { $substrCP: [3, -3], $et: "uoiadb" } } );
   commCompareResults( actResult, expResult2 );

   // pos为负整数，len为0
   actResult = dbcl.find( { a: { $substrCP: [3, 0], $et: "" } } );
   commCompareResults( actResult, expResult0 );

   // pos为0，len大于字符串长度
   actResult = dbcl.find( { a: { $substrCP: [0, 100], $et: "Sequoiadb" } } );
   commCompareResults( actResult, expResult2 );
}