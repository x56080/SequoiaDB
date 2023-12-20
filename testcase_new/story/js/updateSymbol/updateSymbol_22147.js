/******************************************************************************
@Description : seqDB-22147: 使用set更新对象为分区键  
@Modify list : 2020-5-14  Zhao Xiaoni 
*@LastEditTime  : 2023.10.24
*@LastEditors   : tangtao
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = "cs_22147";
testConf.clName = "cl_22147";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "ReplSize": -1 };
main( test );

function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;

   var expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData[i] } );
      expResult.push( { "a": i, "b": allTypeData[i] } );
      testPara.testCL.update( { "$set": { "a": { "$field": "b" } } }, { "a": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, { a: 1 } );
}
