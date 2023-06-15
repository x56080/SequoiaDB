/******************************************************************************
 * @Description   : seqDB-31554:$rightCP 作为选择符
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31554";

main( test )
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 集合插入数据
   var docs = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "Sequoiadb  " },
      { a: "Sequoiadb%^&" },
      { a: "sequoiadb巨杉" },
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

   // {$rightCP: <len> } 
   // value为正整数
   var actResult = dbcl.find( {}, { a: { $rightCP: 3 } } );
   var expResult = [
      { a: "adb" },
      { a: "se" },
      { a: "" },
      { a: "b  " },
      { a: "%^&" },
      { a: "b巨杉" },
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

   // value为负整数
   actResult = dbcl.find( {}, { a: { $rightCP: -3 } } );
   expResult = [
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
      { a: "" },
    ];
   commCompareResults( actResult, expResult );

   // value为0
   actResult = dbcl.find( {}, { a: { $rightCP: 0 } } );
   commCompareResults( actResult, expResult );

   // value大于字符串长度
   actResult = dbcl.find( {}, { a: { $rightCP: 100 } } );
   expResult = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "Sequoiadb  " },
      { a: "Sequoiadb%^&" },
      { a: "sequoiadb巨杉" },
      { a: ["Sequoiadb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "éè𐍈𝄞𠮷" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult );
}