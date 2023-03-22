/***************************************************************************************************
 * @Description: $round功能测试
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
 *    $round-decimal测试
 * 测试步骤：
 *    1. $round取值1，发起查询
 *    2. $round取值0，发起查询
 *    3. $round取值-1，发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_round_3";

main(test);
function test(args)
{
   var cl = args.testCL ;
   cl.alter( { StrictDataMode: false } ) ;

   insertRecords( cl );
   var expRecords = expectRecordsfor1() ;
   var actRecords = cl.find( {}, { "fieldName": { "$round": 1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = expectRecordsfor0() ;
   var actRecords = cl.find( {}, { "fieldName": { "$round": 0 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var actRecords = cl.find( {}, { "fieldName": { "$round": -1 } } ) ;
   var expRecords = expectRecords() ;
   commCompareResults( actRecords, expRecords ) ;

}

function insertRecords ( cl )
{
   var values = [ { "$decimal": "-922337203685477580.8" }, { "$decimal": "922337203685477580.7" },
      { "$decimal": "-92233720368547758.08" }, { "$decimal": "92233720368547758.07" } ];

   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   cl.insert( docs );
}

function expectRecordsfor1()
{
   var values = [ { "$decimal": "-922337203685477580.8" }, { "$decimal": "922337203685477580.7" },
      { "$decimal": "-92233720368547758.1" }, { "$decimal": "92233720368547758.1" } ];
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
   var values = [ { "$decimal": "-922337203685477581" }, { "$decimal": "922337203685477581" },
      { "$decimal": "-92233720368547758" }, { "$decimal": "92233720368547758" } ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

function expectRecords()
{
   var values = [ { "$decimal": "-922337203685477580" }, { "$decimal": "922337203685477580" },
      { "$decimal": "-92233720368547760" }, { "$decimal": "92233720368547760" } ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}


