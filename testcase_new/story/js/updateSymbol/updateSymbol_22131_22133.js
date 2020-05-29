/************************************
*@Description: seqDB-22131:使用inc更新对象为数值，$field指定字段值为非数值
               seqDB-22133:使用inc更新对象为非数值类型 
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clOpt = { ShardingKey : {no :1} };
testConf.clName = COMMCLNAME + "_update_field_22131";
main(test);

function test(testPara)
{
   var docs =[{ no: 0, a: -2, testb:"test0",num:0},
   { no: 1, a: {$numberLong: "9223372036854775000"}, testb:"test1", num: 123},
   { no: 2, a: { num1: 20, num2:0 }, testb:"test2", num:4},
   { no: 3, a: ["test0", 10.23 ,0], testb:"test3",num:-234}];
   insertData( testPara.testCL, docs );
   
   //$field指定字段值为非数值，更新字段为数值
   var updateCondition1 = { $inc: { a: { $field: 'testb'} } };
   var findCondition1 = { no: { $lt: 2 } };
   updateError( updateCondition1, findCondition1 );
   
   //$field指定字段值为非数值，更新字段为对象，数组
   var updateCondition2 = { $inc: { 'a.num1': { $field: 'testb'} } };
   var findCondition2 = { no: { $et: 2 } };
   updateError( updateCondition2, findCondition2 );
   
   var updateCondition3 = { $inc: { 'a.1': { $field: 'testb'} } };
   var findCondition3 = { no: { $et: 3 } };
   updateError( updateCondition3, findCondition3 );
}

function updateError( updateCondition, findCondition )
{
   try
   {
      testPara.testCL.update( updateCondition, findCondition );
      throw new Error( "need throw error" );
   }
   catch(e)
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }
}

