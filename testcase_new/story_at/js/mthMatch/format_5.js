/***************************************************************************************************
 * @Description: $format功能测试
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/16/2022 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    验证$format - 与匹配符配合使用
 * 测试步骤：
 *    1. $format取值2，发起查询
 *    2. $format取值1，发起查询
 *    3. $format取值0，发起查询
 *    4. $format取值-1，发起查询
 * 期望结果：
 *    期望结果与实际结果一致，能解析字符串和布尔值，其他非数值返回 null
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_format_5";

main(test);
function test(args)
{
   var cl = args.testCL ;

   insertRecords(cl) ;

   var expRecords = expectRecordsPart1();
   var actRecords = cl.find( { "fieldName": { "$format": 2, "$et": "-1.00" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart2();
   var actRecords = cl.find( { "fieldName": { "$format": 2, "$et": "1.00" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart3();
   var actRecords = cl.find( { "fieldName": { "$format": 2, "$et": "0.00" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart1();
   var actRecords = cl.find( { "fieldName": { "$format": 1, "$et": "-1.0" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart2();
   var actRecords = cl.find( { "fieldName": { "$format": 1, "$et": "1.0" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart3();
   var actRecords = cl.find( { "fieldName": { "$format": 1, "$et": "0.0" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart1();
   var actRecords = cl.find( { "fieldName": { "$format": 0, "$et": "-1" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart2();
   var actRecords = cl.find( { "fieldName": { "$format": 0, "$et": "1" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart3();
   var actRecords = cl.find( { "fieldName": { "$format": 0, "$et": "0" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart1();
   var actRecords = cl.find( { "fieldName": { "$format": -1, "$et": "-1" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart2();
   var actRecords = cl.find( { "fieldName": { "$format": -1, "$et": "1" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsPart3();
   var actRecords = cl.find( { "fieldName": { "$format": -1, "$et": "0" } } ) ;
   commCompareResults( actRecords, expRecords ) ;

}

function insertRecords ( cl )
{
   var values = [ -1,{ "$numberLong": "-1" }, -1.0, -1e0, "-1","-1.0", "-1.", "-1e0",
      1,{ "$numberLong": "1" }, 1.0, 1e0, "1","1.0", "1.", "1e0", true, false ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   cl.insert( docs );
}

function expectRecordsPart1 ()
{
   var values = [ -1, -1, -1, -1, "-1","-1.0", "-1.", "-1e0" ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecordsPart2 ()
{
   var values = [ 1, 1, 1, 1, "1","1.0", "1.", "1e0", true ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i+8, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecordsPart3 ()
{
   var values = [ false ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i+17, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

