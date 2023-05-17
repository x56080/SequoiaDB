/******************************************************************************
 * @Description   : seqDB-31365:$round函数参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.08
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31365";

main( test )
function test ( testPara )
{
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: 13.14 },
      { No: 2, a: -13.14 },
      { No: 3, a: 0 },
      { No: 4, a: 3.15 },
      { No: 5, a: 123.155 }
   ];
   cl.insert( docs );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$round": "str" } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$round": null } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$round": { $date: "2000-01-01" } } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$round": [1, 2, 3] } } ).toArray();
   } );

   var actRecords = cl.find( {}, { "a": { "$round": -1.2 } } );
   var expRecords1 = [
      { No: 1, a: 10 },
      { No: 2, a: -10 },
      { No: 3, a: 0 },
      { No: 4, a: 0 },
      { No: 5, a: 120 }
   ];
   commCompareResults( actRecords, expRecords1 );

   var actRecords = cl.find( {}, { "a": { "$round": 1.2 } } );
   var expRecords2 = [
      { No: 1, a: 13.1 },
      { No: 2, a: -13.1 },
      { No: 3, a: 0 },
      { No: 4, a: 3.2 },
      { No: 5, a: 123.2 }
   ];
   commCompareResults( actRecords, expRecords2 );

   var actRecords = cl.find( {}, { "a": { "$round": { "$numberLong": "-1" } } } );
   commCompareResults( actRecords, expRecords1 );

   var actRecords = cl.find( {}, { "a": { "$round": { "$numberLong": "1" } } } );
   commCompareResults( actRecords, expRecords2 );

   var actRecords = cl.find( {}, { "a": { "$round": { "$decimal": "-1" } } } );
   commCompareResults( actRecords, expRecords1 );

   var actRecords = cl.find( {}, { "a": { "$round": { "$decimal": "1" } } } );
   commCompareResults( actRecords, expRecords2 );

   var actRecords = cl.find( {}, { "a": { "$round": -2 } } );
   var expRecords = [
      { No: 1, a: 0 },
      { No: 2, a: 0 },
      { No: 3, a: 0 },
      { No: 4, a: 0 },
      { No: 5, a: 100 }
   ];
   commCompareResults( actRecords, expRecords );

   var actRecords = cl.find( {}, { "a": { "$round": 2 } } );
   expRecords = [
      { No: 1, a: 13.14 },
      { No: 2, a: -13.14 },
      { No: 3, a: 0 },
      { No: 4, a: 3.15 },
      { No: 5, a: 123.16 }
   ];
   commCompareResults( actRecords, expRecords );
}