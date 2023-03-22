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
 *    验证$format基本功能
 * 测试步骤：
 *    1. $format取值1，发起查询
 *    2. $format取值0，发起查询
 *    3. $format取值-1，发起查询
 *    4. $format取值16383，发起查询
 *    5. $format取值16384，发起查询
 * 期望结果：
 *    期望结果与实际结果一致，能解析字符串和布尔值，其他非数值返回 null
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_format_1";

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

   cl.find({}, { "fieldName": { "$format": 16383 } }).current() ;
   try
   {
      cl.find({}, { "fieldName": { "$format": 16384 } }).current() ;
      throw new Error( "need throw error" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}

function insertRecords ( cl )
{
   var values = [ 2147483647, { "$numberLong": "-9223372036854775808" },
      1.7e+308, { "$decimal": "9223372036854775808" },
      "9223372036854775807", true, false, { "obj" : 1 }, NaN, -Infinity, Infinity,
      { "$decimal": "nan" }, { "$decimal": "min" }, { "$decimal": "max" },
      null, { "$timestamp":"2012-05-12-13.15.21.241523" },
      [ { "$binary" : "aGVsbG8gd29ybGQ=" } , +1.0, ["arr"] ] ] ;

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
   var values = [ "2,147,483,647", "-9,223,372,036,854,775,808",
      "170,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000",
      "9,223,372,036,854,775,808", "9,223,372,036,854,775,807", "1", "0",
      null, "nan", "-inf", "inf", "NaN", "MIN", "MAX",
      null, null, [null, "1", null ] ] ;

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
   var values = [ "2,147,483,647.0", "-9,223,372,036,854,775,808.0",
      "170,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000.0",
      "9,223,372,036,854,775,808.0", "9,223,372,036,854,775,807.0", "1.0", "0.0",
      null, "nan", "-inf", "inf", "NaN", "MIN", "MAX",
      null, null, [null, "1.0", null ] ] ;

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

