/******************************************************************************
 * @Description   : seqDB-31561:$leftCP 作为匹配符使用
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31561";

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

   // 1.{ $leftCP: <value> } 
   var actResult = dbcl.find( { a: { $leftCP: 3, $et: "Seq" } } );
   var expResult = [
      { a: "Sequoiadb" },
      { a: ["Sequoiadb", 111] }
   ]
   commCompareResults( actResult, expResult );

   actResult = dbcl.find( { a: { $leftCP: -3, $et: "" } } );
   var expResult = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "  Sequoiadb" },
      { a: "Se%^&quoiadb" },
      { a: "巨杉sequoiadb" },
      { a: ["Sequoiadb", 111] },
      { a: "éè𐍈𝄞𠮷" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult );

   actResult = dbcl.find( { a: { $leftCP: 0, $et: "" } } );
   expResult0 = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "  Sequoiadb" },
      { a: "Se%^&quoiadb" },
      { a: "巨杉sequoiadb" },
      { a: ["Sequoiadb", 111] },
      { a: "éè𐍈𝄞𠮷" },
      { a: ".?\\$`" }
   ]
   commCompareResults( actResult, expResult0 );

   actResult = dbcl.find( { a: { $leftCP: 100, $et: "Sequoiadb" } } );
   expResult = [
      { a: "Sequoiadb" },
      { a: ["Sequoiadb", 111] }
   ]
   commCompareResults( actResult, expResult );
}