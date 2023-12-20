/******************************************************************************
@Description : seqDB-22143: 使用set更新符，$field指定字段更新 
@Modify list : 2020-5-14  Zhao Xiaoni 
*@LastEditTime  : 2023.10.24
*@LastEditors   : tangtao
******************************************************************************/
testConf.csName = "cs_22143";
testConf.clName = "cl_22143";
testConf.clOpt = { ReplSize: -1 };
main( test );

function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;

   var expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData[i], "c": i } );
      expResult.push( { "a": allTypeData[i], "b": allTypeData[i], "c": i } );
      testPara.testCL.update( { "$set": { "a": { "$field": "b" } } }, { "a": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, { c: 1 } );
}
