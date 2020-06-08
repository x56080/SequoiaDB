/************************************
*@Description: seqDB-22142:匹配不到记录，upsert使用inc更新符 
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22142";
main(test);

function test(testPara)
{
   var doc1 =[{ no: 0, a: 0, fieldb: -21470, testc:"test0"},
   { no: 1, a: 1, fieldb: {$numberLong: "9223372036854775800"}, testc:"test1"},
   { no: 2, a: -234, a1: 20, fieldb: { num1: 20, num2: 0 }, fieldb1:{ num: {a: 12.01}}, testc:"test4"},
   { no: 3, a: -10.23, a1: 1203333, fieldb: ["test0", 10.23 ,[0, -123]], testc:"test5"}];
   insertData( testPara.testCL, doc1 );   

   var updateCondition1 = { $inc: { testa1: { $field: 'fieldb'}} };
   var findCondition1 = { testa1: -21470 };
   upsertData( testPara.testCL, updateCondition1, findCondition1 );  
   doc1.push({ testa1: -21470 });   
   checkResult( testPara.testCL, null, null, doc1, { _id: 1 } );
   
   //$field指定字段值为对象类型
   var updateCondition2 = { $inc: { testa2: { $field: 'fieldb.num1'} ,testb2: { $field: 'fieldb.num2.a'}}};
   var findCondition2 = { fieldb: { num1: 20, num2: {a: 12.01} } };
   upsertData( testPara.testCL, updateCondition2, findCondition2 );   
   doc1.push({ testa2: 20, testb2:12.01 ,fieldb: { num1: 20, num2: {a: 12.01} }});
   checkResult( testPara.testCL, null, null, doc1, { _id: 1 } );
   
   //$field指定字段值为数组类型
   var updateCondition3 = { $inc: { testa3: { $field: 'field.1'}, testb3: { $field: 'field.2.1'} } };
   var findCondition3 = { field: ["test0", 10.23 ,[0, -123]] };
   upsertData( testPara.testCL, updateCondition3, findCondition3 );   
   doc1.push({ testa3: 10.23, testb3:-123 ,field: ["test0", 10.23 ,[0, -123]]});
   checkResult( testPara.testCL, null, null, doc1, { _id: 1 } );
}

