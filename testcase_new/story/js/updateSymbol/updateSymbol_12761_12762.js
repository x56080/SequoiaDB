/************************************
*@Description: seqDB-12761:update使用pull_all_by更新非数组对象
               seqDB-12762:update使用pull_all_by更新空数组对象
*@author:      liuxiaoxuan
*@createdate:  2017.09.19
*@LastEditTime  : 2023.10.24
*@LastEditors   : tangtao
**************************************/
testConf.csName = COMMCSNAME + "_pull_all_by_12761";
testConf.clName = COMMCLNAME + "_pull_all_by_12761";
testConf.clOpt = { ReplSize: -1 };
main( test );

function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;

   //insert data   
   var doc = [{ a1: 1 },
   { a2: 'aaa' },
   { a3: [] }];
   testPara.testCL.insert( doc );

   //pull_by
   var updateRule = { $pull_all_by: { a1: [1], a2: ['aaa'], a3: [[]] } };
   testPara.testCL.update( updateRule );

   //check result
   var expResult = [{ a1: 1 },
   { a2: 'aaa' },
   { a3: [] }];
   checkResult( testPara.testCL, null, null, expResult, { _id: 1 } );
   checkResultSync( csName, clName, null, null, expResult, { _id: 1 } );
}

