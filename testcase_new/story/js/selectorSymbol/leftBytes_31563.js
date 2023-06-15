/******************************************************************************
 * @Description   : seqDB-31563:$leftBytes 作为选择符
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31563";

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

   // {$lefttBytes: <len> } 
   // value为正整数
   var actResult = dbcl.find( {}, { a: { $leftBytes: 6 } } );
   var expResult = [
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
   commCompareResults( actResult, expResult );

   // value为负整数
   actResult = dbcl.find( {}, { a: { $leftBytes: -6 } } );
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
   actResult = dbcl.find( {}, { a: { $leftBytes: 0 } } );
   commCompareResults( actResult, expResult );

   // value大于字符串长度
   actResult = dbcl.find( {}, { a: { $leftBytes: 100 } } );
   expResult = [
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
   commCompareResults( actResult, expResult );
}