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
 *    验证$format - int/long
 * 测试步骤：
 *    1. $format取值1，发起查询
 *    2. $format取值0，发起查询
 *    3. $format取值-1，发起查询
 * 期望结果：
 *    期望结果与实际结果一致，格式化输出字符串
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_format_2";

main(test);
function test(args)
{
   var cl = args.testCL ;

   insertRecords(cl) ;
   var expRecords = expectRecords1();

   var actRecords = cl.find( {}, { "fieldName": { "$format": 1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecords();
   var actRecords = cl.find( {}, { "fieldName": { "$format": 0 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( {}, { "fieldName": { "$format": -1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

}

function insertRecords ( cl )
{
   var values = [ 0, 1, -1, 2147483647, -2147483648, 100000, -100000,
      { "$numberLong": "-9223372036854775808" }, { "$numberLong": "9223372036854775807" },
      { "$numberLong": "100000" }, { "$numberLong": "-100000" } ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   cl.insert( docs );
}

function expectRecords ()
{
   var values = [ "0", "1", "-1", "2,147,483,647" ,"-2,147,483,648", "100,000", "-100,000",
      "-9,223,372,036,854,775,808", "9,223,372,036,854,775,807", "100,000", "-100,000" ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecords1 ()
{
   var values = [ "0.0", "1.0", "-1.0", "2,147,483,647.0" ,"-2,147,483,648.0", "100,000.0", "-100,000.0",
      "-9,223,372,036,854,775,808.0", "9,223,372,036,854,775,807.0", "100,000.0", "-100,000.0" ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

