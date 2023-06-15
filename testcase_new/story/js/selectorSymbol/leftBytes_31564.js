/******************************************************************************
 * @Description   : seqDB-31564:$leftBytes作为匹配符使用
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31564";

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

   // {$leftBytes: <len> } 
   // value为正整数
   var actResult = dbcl.find( { a: { $leftBytes: 6, $et: "Sequoi" } } );
   var expResult = [
      { a: "Sequoiadb" },
      { a: ["Sequoiadb", 111] }
   ]
   commCompareResults( actResult, expResult );

   // value为负整数
   actResult = dbcl.find( { a: { $leftBytes: -6, $et: "" } } );
   var expResult = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "  Sequoiadb" },
      { a: "Se%^&quoiadb" },
      { a: "巨杉sequoiadb" },
      { a: ["Sequoiadb", 111] },
      { a: "éèéèéè" },
      { a: ".?\\$`" }
   ];
   commCompareResults( actResult, expResult );

   // value为0
   actResult = dbcl.find( { a: { $leftBytes: 0, $et: "" } } );
   expResult0 = [
      { a: "Sequoiadb" },
      { a: "se" },
      { a: "" },
      { a: "  Sequoiadb" },
      { a: "Se%^&quoiadb" },
      { a: "巨杉sequoiadb" },
      { a: ["Sequoiadb", 111] },
      { a: "éèéèéè" },
      { a: ".?\\$`" }
   ]
   commCompareResults( actResult, expResult0 );

   // value大于字符串长度
   actResult = dbcl.find( { a: { $leftBytes: 100, $et: "Sequoiadb" } } );
   expResult = [
      { a: "Sequoiadb" },
      { a: ["Sequoiadb", 111] }
   ]
   commCompareResults( actResult, expResult );
}