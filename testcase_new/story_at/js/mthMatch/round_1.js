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
 *    验证$round非数值和特殊值
 * 测试步骤：
 *    1. $round取值1，发起查询
 *    2. $round取值0，发起查询
 *    3. $round取值-1，发起查询
 *    4. $round取值-308，发起查询
 *    5. $round取值-309，发起查询
 * 期望结果：
 *    期望结果与实际结果一致，非数值返回null，无穷和NAN无变化
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_round_1";

main(test);
function test(args)
{
   var cl = args.testCL ;

   insertRecords(cl) ;
   var expRecords = expectRecords1();

   var actRecords = cl.find( {}, { "fieldName": { "$round": 1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( {}, { "fieldName": { "$round": 0 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( {}, { "fieldName": { "$round": -1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( {}, { "fieldName": { "$round": -308 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( {}, { "fieldName": { "$round": -309 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

}

function insertRecords ( cl )
{
   var values = [ { "$decimal": "MIN" }, { "$decimal": "MAX" },
      { "$decimal": "NaN" }, -Infinity, Infinity, NaN, "str" ];

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   cl.insert( docs );
}

function expectRecords1 ()
{
   var values = [ { "$decimal": "MIN" }, { "$decimal": "MAX" },
      { "$decimal": "NaN" }, -Infinity, Infinity, NaN, null ];

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

