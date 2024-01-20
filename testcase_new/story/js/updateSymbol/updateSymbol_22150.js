/******************************************************************************
@Description :  seqDB-22150: 使用pop指定field字段更新对象，field指定不同类型值  
@Modify list : 2020-5-14  Zhao Xiaoni 
*@LastEditTime  : 2023.10.24
*@LastEditors   : tangtao
******************************************************************************/
testConf.csName = "cs_22150";
testConf.clName = "cl_22150";
testConf.clOpt = { ReplSize: -1 };
main( test );

function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;

   //b字段为0
   var expResult = [{ "a": [1, 2, 3], "b": 0 }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": 0 } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
   testPara.testCL.remove();

   //b字段为正数
   expResult = [{ "a": [1, 2], "b": 1 }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": 1 } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
   testPara.testCL.remove();

   //b字段为负数   
   expResult = [{ "a": [2, 3], "b": -1 }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": -1 } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
   testPara.testCL.remove();

   //b字段为嵌套数组
   expResult = [{ "a": [1], "b": [[1, -1], [2, -2]] }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": [[1, -1], [2, -2]] } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b.1.0" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
   testPara.testCL.remove();

   //b字段为嵌套对象
   expResult = [{ "a": [1, 2], "b": { "parent": { "child": 1 } } }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": { "parent": { "child": 1 } } } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b.parent.child" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   checkResultSync( csName, clName, null, null, expResult, null );
}
