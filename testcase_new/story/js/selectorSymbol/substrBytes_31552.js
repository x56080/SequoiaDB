/******************************************************************************
 * @Description   : seqDB-31552 : $substrBytes 作为匹配符
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.09
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31552";

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
   var actResult = dbcl.find( { a: { $substrBytes: 6, $et: "Sequoi" } } );
   var expResult6 = [
      { a: "Sequoiadb" },
      { a: ["Sequoiadb", 111] }
   ];
   commCompareResults( actResult, expResult6 );
   
   // value为负整数
   actResult = dbcl.find( { a: { $substrBytes: -6, $et: "uoiadb" } } );
   var expResult_6 = [
      { "a": "Sequoiadb" },
      { "a": "  Sequoiadb" },
      { "a": "Se%^&quoiadb" },
      { "a": "巨杉sequoiadb" },
      { "a": ["Sequoiadb", 111] }
   ]
   commCompareResults( actResult, expResult_6 );

   // value为0
   actResult = dbcl.find( { a: { $substrBytes: 0, $et: "" } } );
   expResult0 = [
      { "a": "Sequoiadb" },
      { "a": "se" },
      { "a": "" },
      { "a": "  Sequoiadb" },
      { "a": "Se%^&quoiadb" },
      { "a": "巨杉sequoiadb" },
      { "a": ["Sequoiadb", 111] },
      { a: "éèéèéè" },
      { "a": ".?\\$`" }
   ]
   commCompareResults( actResult, expResult0 );

   // value大于字符串长度
   actResult = dbcl.find( { a: { $substrBytes: 100, $et: "Sequoiadb" } } );
   commCompareResults( actResult, expResult6 );

   // { $substrBytes: [<pos>, <len>] }
   // pos为正整数，len为正整数
   actResult = dbcl.find( { a: { $substrBytes: [6, 2], $et: "è" } } );
   commCompareResults( actResult, [{"a":"éèéèéè"}] );

   // pos为负整数，len为正整数
   actResult = dbcl.find( { a: { $substrBytes: [-6, 6], $et: "uoiadb" } } );
   commCompareResults( actResult, expResult_6 );

   // pos为0，len为正整数
   actResult = dbcl.find( { a: { $substrBytes: [0, 6], $et: "巨杉" } } );
   commCompareResults( actResult, [{ a: "巨杉sequoiadb" }] );

   // pos大于字符串长度，len为正整数
   actResult = dbcl.find( { a: { $substrBytes: [100, 3], $et: "" } } );
   commCompareResults( actResult, expResult0 );

   // pos为正整数，len为负整数
   actResult = dbcl.find( { a: { $substrBytes: [6, -2], $et: "èéè" } } );
   commCompareResults( actResult, [{"a":"éèéèéè"}] );

   // pos为负整数，len为0
   actResult = dbcl.find( { a: { $substrBytes: [6, 0], $et: "" } } );
   commCompareResults( actResult, expResult0 );

   // pos为0，len大于字符串长度
   actResult = dbcl.find( { a: { $substrBytes: [0, 100], $et: "Sequoiadb" } } );
   commCompareResults( actResult, expResult6 );
}