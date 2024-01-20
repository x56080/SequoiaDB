/******************************************************************************
@Description : seqDB-22144: 使用set更新对象，$field指定字段为不存在/NULL 
               seqDB-22151: 使用pop更新对象，$field指定字段不存在
               seqDB-22156: 使用pull更新对象，$field指定字段不存在 
               seqDB-22161: 使用pull_by更新对象，$field指定字段不存在 
               seqDB-22166: 使用push更新对象，$field指定字段不存在 
@Modify list : 2020-5-14  Zhao Xiaoni 
*@LastEditTime  : 2023.10.24
*@LastEditors   : tangtao
******************************************************************************/
testConf.csName = "cs_22144_22151_22156_22161_22166";
testConf.clName = "cl_22144_22151_22156_22161_22166";
testConf.clOpt = { ReplSize: -1 };
main( test );

function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;

   //使用set更新对象
   var expResult = [{ "a": null, "c": null }];
   testPara.testCL.insert( { "a": 1, "b": 1, "c": null } );
   testPara.testCL.update( { "$set": { "a": { "$field": "c" }, "b": { "$field": "d" } } } );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
   testPara.testCL.remove();

   //使用pop更新对象
   expResult = [{ "a": [1, 2, 3] }];
   testPara.testCL.insert( { "a": [1, 2, 3] } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );

   //使用pull更新对象
   testPara.testCL.update( { "$pull": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );

   //使用pull_by更新对象
   testPara.testCL.update( { "$pull_by": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );

   //使用push更新对象
   testPara.testCL.update( { "$push": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
}
