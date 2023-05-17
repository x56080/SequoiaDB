/******************************************************************************
 * @Description   : seqDB-31371:$round double调用验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.08
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31371";

main( test )
function test ( testPara )
{
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: 13.14 }, { No: 2, a: 13.441 }, { No: 3, a: 13.542 }, { No: 4, a: 13.643 },
      { No: 5, a: 13.744 }, { No: 6, a: 3.145 }, { No: 7, a: 3.146 }, { No: 8, a: 3.547 },
      { No: 9, a: 3.648 }, { No: 10, a: 3.749 }
   ];
   cl.insert( docs );

   var actRecords = cl.find( {}, { "a": { "$round": -1 } } );
   var expRecords = [
      { No: 1, a: 10 }, { No: 2, a: 10 }, { No: 3, a: 10 }, { No: 4, a: 10 },
      { No: 5, a: 10 }, { No: 6, a: 0 }, { No: 7, a: 0 }, { No: 8, a: 0 },
      { No: 9, a: 0 }, { No: 10, a: 0 }
   ];
   commCompareResults( actRecords, expRecords );

   actRecords = cl.find( {}, { "a": { "$round": 0 } } );
   var expRecords = [
      { No: 1, a: 13 }, { No: 2, a: 13 }, { No: 3, a: 14 }, { No: 4, a: 14 },
      { No: 5, a: 14 }, { No: 6, a: 3 }, { No: 7, a: 3 }, { No: 8, a: 4 },
      { No: 9, a: 4 }, { No: 10, a: 4 }
   ];
   commCompareResults( actRecords, expRecords );

   actRecords = cl.find( {}, { "a": { "$round": 2 } } );
   var expRecords = [
      { No: 1, a: 13.14 }, { No: 2, a: 13.44 }, { No: 3, a: 13.54 }, { No: 4, a: 13.64 },
      { No: 5, a: 13.74 }, { No: 6, a: 3.15 }, { No: 7, a: 3.15 }, { No: 8, a: 3.55 },
      { No: 9, a: 3.65 }, { No: 10, a: 3.75 }
   ];
   commCompareResults( actRecords, expRecords );

   actRecords = cl.find( { "a": { "$round": -1, "$et": 10 } } );
   var expRecords = [
      { No: 1, a: 13.14 }, { No: 2, a: 13.441 }, { No: 3, a: 13.542 }, { No: 4, a: 13.643 },
      { No: 5, a: 13.744 }
   ];
   commCompareResults( actRecords, expRecords );

   actRecords = cl.find( { "a": { "$round": -1, "$et": 0 } } );
   var expRecords = [
      { No: 6, a: 3.145 }, { No: 7, a: 3.146 }, { No: 8, a: 3.547 }, { No: 9, a: 3.648 },
      { No: 10, a: 3.749 }
   ];

   var docs = [
      { No: 11, a: -13.14 }, { No: 12, a: -13.441 }, { No: 13, a: -13.542 }, { No: 14, a: -13.643 }
   ];
   cl.insert( docs );
   actRecords = cl.find( { "a": { "$round": -1, "$et": -10 } } );
   commCompareResults( actRecords, docs );
}