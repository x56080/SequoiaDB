/******************************************************************************
 * @Description   : seqDB-31366:$round函数验证特殊值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.08
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31366";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: { "$decimal": "MIN" } },
      { No: 2, a: { "$decimal": "MAX" } },
      { No: 3, a: { "$decimal": "NaN" } },
      { No: 4, a: -Infinity },
      { No: 5, a: Infinity },
      { No: 6, a: NaN }
   ]
   cl.insert( docs );

   var expRecords = docs;

   var actRecords = cl.find( {}, { "a": { "$round": 1 } } );
   commCompareResults( actRecords, expRecords );

   var actRecords = cl.find( {}, { "a": { "$round": 0 } } );
   commCompareResults( actRecords, expRecords );

   var actRecords = cl.find( {}, { "a": { "$round": -1 } } );
   commCompareResults( actRecords, expRecords );

   var actRecords = cl.find( {}, { "a": { "$round": -10 } } );
   commCompareResults( actRecords, expRecords );

   var actRecords = cl.find( {}, { "a": { "$round": -10 } } );
   commCompareResults( actRecords, expRecords );
}