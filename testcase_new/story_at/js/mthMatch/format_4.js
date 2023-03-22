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
 *    验证$format - string
 * 测试步骤：
 *    1. $format取值2，发起查询
 *    2. $format取值1，发起查询
 *    3. $format取值0，发起查询
 *    4. $format取值-1，发起查询
 * 期望结果：
 *    期望结果与实际结果一致，格式化输出字符串，其他非数值返回 null
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_format_4";

main(test);
function test(args)
{
   var cl = args.testCL ;

   insertRecords(cl) ;
   var expRecords = expectRecords2();

   var actRecords = cl.find( {}, { "fieldName": { "$format": 2 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

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
   var values = [ "1", "10", "1e2", "1e3", "-1", "-10", "-1e2", "-1e3", "1e-2", "-1e-2",
      "1e-3", "-1e-3", "+123,456", "n", "na", "nan", "123.456", ".456", "-.456", "-.e2", "-1.e2", "-e2" ] ;

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
   var values = [ "1", "10", "100", "1,000", "-1", "-10", "-100", "-1,000",
      "0", "-0", "0", "-0", "123", "0", "0", "0", "123", "0", "-0", "0", "-100", "0" ] ;

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
   var values = [ "1.0", "10.0", "100.0", "1,000.0", "-1.0", "-10.0", "-100.0", "-1,000.0", "0.0", "-0.0",
      "0.0", "-0.0", "123.0", "0.0", "0.0", "0.0", "123.5", "0.5", "-0.5", "0.0", "-100.0", "0.0" ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecords2 ()
{
   var values = [ "1.00", "10.00", "100.00", "1,000.00", "-1.00", "-10.00", "-100.00", "-1,000.00", "0.01",
      "-0.01", "0.00", "-0.00", "123.00", "0.00", "0.00", "0.00", "123.46", "0.46", "-0.46", "0.00", "-100.00", "0.00" ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

