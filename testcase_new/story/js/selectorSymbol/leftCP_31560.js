/******************************************************************************
 * @Description   : seqDB-31560:$leftCP 作为选择符
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31560";

main( test )
function test ( testPara )
{
   var dbcl = testPara.testCL;

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

   // {$leftCP: <len> } 
   var actResult = dbcl.find( {}, { a: { $leftCP: 3 } } );
   var expResult = [
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
   commCompareResults( actResult, expResult );

   actResult = dbcl.find( {}, { a: { $leftCP: -3 } } );
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

   actResult = dbcl.find( {}, { a: { $leftCP: 0 } } );
   commCompareResults( actResult, expResult );

   actResult = dbcl.find( {}, { a: { $leftCP: 100 } } );
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
      { a: "éè𐍈𝄞𠮷" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult );
}