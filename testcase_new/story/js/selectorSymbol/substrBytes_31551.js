/******************************************************************************
 * @Description   : seqDB-31551:$substrBytes 作为选择符
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.09
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31551";

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
      { a: "éèéèéè" },
      { a: ".?\\$`" }
   ];
   dbcl.insert( docs );

   // 1. { $substrBytes: <value> } 
   // value为正整数
   var actResult = dbcl.find( {}, { a: { $substrBytes: 6 } } );
   var expResult6 = [
      { a: "Sequoi" },
      { a: "se" },
      { a: "" },
      { a: "  Sequ" },
      { a: "Se%^&q" },
      { a: "巨杉" },
      { a: ["Sequoi", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "éèé" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult6 );

   // value为负整数
   actResult = dbcl.find( {}, { a: { $substrBytes: -6 } } );
   var expResult_6 = [
      { a: "uoiadb" },
      { a: "" },
      { a: "" },
      { a: "uoiadb" },
      { a: "uoiadb" },
      { a: "uoiadb" },
      { a: ["uoiadb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "èéè" },
      { a: "" }
   ];
   commCompareResults( actResult, expResult_6 );

   // value为0
   actResult = dbcl.find( {}, { a: { $substrBytes: 0 } } );
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
   actResult = dbcl.find( {}, { a: { $substrBytes: 100 } } );
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
      { a: "éèéèéè" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult100 );

   // { $substrBytes: [<pos>, <len>] }
   // pos为正整数，len为正整数
   actResult = dbcl.find( {}, { a: { $substrBytes: [6, 2] } } );
   var expResult = [
      { a: "ad" },
      { a: "" },
      { a: "" },
      { a: "oi" },
      { a: "uo" },
      { a: "se" },
      { a: ["ad", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "è" },
      { a: "" }
   ];
   commCompareResults( actResult, expResult );

   // pos为负整数，len为正整数
   actResult = dbcl.find( {}, { a: { $substrBytes: [-6, 6] } } );
   commCompareResults( actResult, expResult_6 );

   // pos为0，len为正整数
   actResult = dbcl.find( {}, { a: { $substrBytes: [0, 6] } } );
   commCompareResults( actResult, expResult6 );

   // pos大于字符串长度，len为正整数
   actResult = dbcl.find( {}, { a: { $substrBytes: [100, 3] } } );
   commCompareResults( actResult, expResult0 );

   // pos为正整数，len为负整数
   actResult = dbcl.find( {}, { a: { $substrBytes: [6, -2] } } );
   var expResult = [
      { a: "adb" },
      { a: "" },
      { a: "" },
      { a: "oiadb" },
      { a: "uoiadb" },
      { a: "sequoiadb" },
      { a: ["adb", null] },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: null },
      { a: "èéè" },
      { a: "" }
   ];
   commCompareResults( actResult, expResult );

   // pos为负整数，len为0
   actResult = dbcl.find( {}, { a: { $substrBytes: [6, 0] } } );
   commCompareResults( actResult, expResult0 );

   // pos为0，len大于字符串长度
   actResult = dbcl.find( {}, { a: { $substrBytes: [0, 100] } } );
   commCompareResults( actResult, expResult100 );
}