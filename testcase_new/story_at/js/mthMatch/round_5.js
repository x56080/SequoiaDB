/***************************************************************************************************
 * @Description: 设置集合同步一致性策略
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/13/2022 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    $round与匹配符配合使用 - int
 * 测试步骤：
 *    1. $round取值2，发起查询
 *    2. $round取值1，发起查询
 *    3. $round取值0，发起查询
 *    4. $round取值-1，发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_round_5";

main(test);
function test(args)
{
   var cl = args.testCL ;
   cl.alter( { StrictDataMode: false } ) ;

   var expRecords = insertRecords( cl );
   var actRecords = cl.find( {},{ "fieldName": { "$round": 2 } } ) ;
   commCompareResults( actRecords, expRecords ) ;
   var actRecords = cl.find( {},{ "fieldName": { "$round": 1} } ) ;
   commCompareResults( actRecords, expRecords ) ;
   var actRecords = cl.find( {},{ "fieldName": { "$round": 0 } } ) ;
   commCompareResults( actRecords, expRecords ) ;


   var expRecords = expectRecords()
   var actRecords = cl.find( { "fieldName": { "$round": -1, "$et": -10 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsfor0()
   var actRecords = cl.find( { "fieldName": { "$round": -1, "$et": 0 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsfor10()
   var actRecords = cl.find( { "fieldName": { "$round": -1, "$et": 10 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

}

function insertRecords ( cl )
{
   var values = [ -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   cl.insert( docs );
   return docs ;
}

function expectRecords()
{
   var values = [ -9, -8, -7, -6, -5 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecordsfor0()
{
   var values = [ -4,-3,-2,-1, 0, 1, 2, 3, 4 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i+5, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecordsfor10()
{
   var values = [ 5, 6, 7, 8, 9 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i+14, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}


