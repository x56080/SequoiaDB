/***************************************************************************************************
 * @Description: 设置集合同步一致性策略
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/06/2022 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    $round与匹配符配合使用
 * 测试步骤：
 *    1. $round取值-1，发起查询
 *    2. $round取值0，发起查询
 *    3. $round取值1，发起查询
 *    3. $round取值2，发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_round_4";

main(test);
function test(args)
{
   var cl = args.testCL ;
   cl.alter( { StrictDataMode: false } ) ;

   var expRecords = insertRecords( cl );
   var actRecords = cl.find( { "fieldName": { "$round": -1, "$et": 0 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( { "fieldName": { "$round": 0, "$et": 3 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( { "fieldName": { "$round": 1, "$et": 3.1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [] ;
   var actRecords = cl.find( { "fieldName": { "$round": 1, "$et": 3.2 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsET3p14() ;
   var actRecords = cl.find( { "fieldName": { "$round": 2, "$et": 3.14 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsET3p15() ;
   var actRecords = cl.find( { "fieldName": { "$round": 2, "$et": 3.15 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

}

function insertRecords ( cl )
{
   var values = [ 3.14, 3.141, 3.142, 3.143, 3.144, 3.145, 3.146, 3.147, 3.148, 3.149 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   cl.insert( docs );
   return docs;
}

function expectRecordsET3p14()
{
   var values = [ 3.14, 3.141, 3.142, 3.143, 3.144 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecordsET3p15()
{
   var values = [ 3.145, 3.146, 3.147, 3.148, 3.149 ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i+5, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

