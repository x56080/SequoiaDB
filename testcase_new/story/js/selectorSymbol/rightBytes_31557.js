/******************************************************************************
 * @Description   : seqDB-31557:$rightBytes 作为选择符
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31557";

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
      { a: "éèéèéè" },
      { a: ".?\\$`" }
   ];
   dbcl.insert( docs );

   // {$rightBytes: <len> } 
   // value为正整数
   var actResult = dbcl.find( {}, { a: { $rightBytes: 6 } } );
   var expResult = [
      { a: "uoiadb" },
      { a: "se" },
      { a: "" },
      { a: "iadb  " },
      { a: "adb%^&" },
      { a: "巨杉" },
      { a: ["uoiadb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "èéè" },
      {"a":".?\\$`"}
   ];
   commCompareResults( actResult, expResult );

   // value为负整数
   actResult = dbcl.find( {}, { a: { $rightBytes: -6 } } );
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
   actResult = dbcl.find( {}, { a: { $rightBytes: 0 } } );
   commCompareResults( actResult, expResult );

   // value大于字符串长度
   actResult = dbcl.find( {}, { a: { $rightBytes: 100 } } );
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
      { a: "éèéèéè" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult );
}