/******************************************************************************
 * @Description   : seqDB-31547:$substrCP作为选择符使用
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.09
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31547";

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
   var actResult = dbcl.find( {}, { a: { $substrCP: 3 } } );
   var expResult3 = [
      { a: "Seq" },
      { a: "se" },
      { a: "" },
      { a: "  S" },
      { a: "Se%" },
      { a: "巨杉s" },
      { a: ["Seq", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "éè𐍈" },
      { a: ".?\\" }
   ];
   commCompareResults( actResult, expResult3 );

   // value为负整数
   actResult = dbcl.find( {}, { a: { $substrCP: -3 } } );
   var expResult = [
      { a: "adb" },
      { a: "" },
      { a: "" },
      { a: "adb" },
      { a: "adb" },
      { a: "adb" },
      { a: ["adb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "𐍈𝄞𠮷" },
      { a: "\\$`" }
   ];
   commCompareResults( actResult, expResult );

   // value为0
   actResult = dbcl.find( {}, { a: { $substrCP: 0 } } );
   var expResult0 = [
      { a: "" },
      { a: "" },
      { a: "" },
      { a: "" },
      { a: "" },
      { a: "" },
      { a: ["", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "" },
      { a: "" }
   ];
   commCompareResults( actResult, expResult0 );

   // value大于字符串长度
   actResult = dbcl.find( {}, { a: { $substrCP: 100 } } );
   var expResult100 = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "  Sequoiadb" },
      { a: "Se%^&quoiadb" },
      { a: "巨杉sequoiadb" },
      { a: ["Sequoiadb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "éè𐍈𝄞𠮷" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult100 );

   // { $substrCP: [<pos>, <len>] }
   // pos为正整数，len为正整数
   actResult = dbcl.find( {}, { a: { $substrCP: [3, 3] } } );
   var expResult = [
      { a: "uoi" },
      { a: "" },
      { a: "" },
      { a: "equ" },
      { a: "^&q" },
      { a: "equ" },
      { a: ["uoi", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "𝄞𠮷" },
      { a: "$`" }
   ];
   commCompareResults( actResult, expResult );

   // pos为负整数，len为正整数
   actResult = dbcl.find( {}, { a: { $substrCP: [-3, 3] } } );
   var expResult = [
      { a: "adb" },
      { a: "" },
      { a: "" },
      { a: "adb" },
      { a: "adb" },
      { a: "adb" },
      { a: ["adb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "𐍈𝄞𠮷" },
      { a: "\\$`" }
   ];
   commCompareResults( actResult, expResult );

   // pos为0，len为正整数
   actResult = dbcl.find( {}, { a: { $substrCP: [0, 3] } } );
   commCompareResults( actResult, expResult3 );

   // pos大于字符串长度，len为正整数
   actResult = dbcl.find( {}, { a: { $substrCP: [100, 3] } } );
   commCompareResults( actResult, expResult0 );

   // pos为正整数，len为负整数
   actResult = dbcl.find( {}, { a: { $substrCP: [3, -3] } } );
   var expResult = [
      { a: "uoiadb" },
      { a: "" },
      { a: "" },
      { a: "equoiadb" },
      { a: "^&quoiadb" },
      { a: "equoiadb" },
      { a: ["uoiadb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "𝄞𠮷" },
      { a: "$`" }
   ];
   commCompareResults( actResult, expResult );

   // pos为负整数，len为0
   actResult = dbcl.find( {}, { a: { $substrCP: [3, 0] } } );
   commCompareResults( actResult, expResult0 );

   // pos为0，len大于字符串长度
   actResult = dbcl.find( {}, { a: { $substrCP: [0, 100] } } );
   commCompareResults( actResult, expResult100 );
}