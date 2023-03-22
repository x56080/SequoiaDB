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
 *    $round功能测试
 * 测试步骤：
 *    1. $round取值1，发起查询
 *    2. $round取值0，发起查询
 *    3. $round取值-1，发起查询
 *    4. $round取值-10，查询32位整型最大值、最小值
 *    5. $round取值-19，查询64位整型最大值、最小值
 *    6. $round取值-308，查询64位浮点型最大值、最小值
 * 期望结果：
 *    期望结果与实际结果一致，非数值返回null，无穷和NAN无变化
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_round_2";

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

   var expRecords = expectRecords() ;
   var actRecords = cl.find( {}, { "fieldName": { "$round": -1 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [{ "no": 0, "fieldName": 0 }];
   var actRecords = cl.find( { "no": 0 }, { "fieldName": { "$round": -10 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [{ "no": 1, "fieldName": 0 }];
   var actRecords = cl.find( { "no": 1 }, { "fieldName": { "$round": -10 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [{ "no": 2, "fieldName": { "$decimal": "-10000000000000000000" } }];
   var actRecords = cl.find( { "no": 2 }, { "fieldName": { "$round": -19 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [{ "no": 3, "fieldName": { "$decimal": "10000000000000000000" } }];
   var actRecords = cl.find( { "no": 3 }, { "fieldName": { "$round": -19 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [{ "no": 4, "fieldName": { "$decimal": "-200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } }];
   var actRecords = cl.find( { "no": 4 }, { "fieldName": { "$round": -308 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   var expRecords = [{ "no": 5, "fieldName": { "$decimal": "200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } }];
   var actRecords = cl.find( { "no": 5 }, { "fieldName": { "$round": -308 } } ) ;
   commCompareResults( actRecords, expRecords ) ;

   cl.alter( { StrictDataMode: true } ) ;
   for( var i = 0; i < 4; ++i )
   {
      try
      {
         cl.find( { "no": i }, { "fieldName": { "$round": -1 } } ).current() ;
         throw new Error( "need throw error" );
      }
      catch( e )
      {
         if( e.message != SDB_VALUE_OVERFLOW )
         {
            throw e;
         }
      }
   }
   for( var i = 4; i < 7; ++i )
   {
      cl.find( { "no": i }, { "fieldName": { "$round": -1 } } ).current() ;
   }
   cl.find( { "no": 0 }, { "fieldName": { "$round": -10 } } ).current() ;
   cl.find( { "no": 1 }, { "fieldName": { "$round": -10 } } ).current() ;
   try
   {
      cl.find( { "no": 2 }, { "fieldName": { "$round": -19 } } ).current() ;
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_VALUE_OVERFLOW )
      {
         throw e;
      }
   }
   try
   {
      cl.find( { "no": 3 }, { "fieldName": { "$round": -19 } } ).current() ;
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_VALUE_OVERFLOW )
      {
         throw e;
      }
   }
   try
   {
      cl.find( { "no": 4 }, { "fieldName": { "$round": -308 } } ).current() ;
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_VALUE_OVERFLOW )
      {
         throw e;
      }
   }
   try
   {
      cl.find( { "no": 5 }, { "fieldName": { "$round": -308 } } ).current() ;
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_VALUE_OVERFLOW )
      {
         throw e;
      }
   }
}

function insertRecords ( cl )
{
   var values = [-2147483648, 2147483647, { "$numberLong": "-9223372036854775808" },
      { "$numberLong": "9223372036854775807" }, -1.7E+308, 1.7e+308, { "$decimal": "9223372036854775808" } ];

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
   var values = [ -2147483648, 2147483647, { "$numberLong": "-9223372036854775808" },
      { "$numberLong": "9223372036854775807" }, -1.7E+308, 1.7e+308, { "$decimal": "9223372036854775808.0" } ];
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
   var values = [ -2147483648, 2147483647, { "$numberLong": "-9223372036854775808" },
      { "$numberLong": "9223372036854775807" }, -1.7E+308, 1.7e+308, { "$decimal": "9223372036854775808" } ];
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
   var values = [ -2147483650, 2147483650, { "$decimal": "-9223372036854775810" },
      { "$decimal": "9223372036854775810" }, -1.7E+308, 1.7e+308, { "$decimal": "9223372036854775810" } ];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var fieldValue = values[i];
      var objs = { "no": i, "fieldName": fieldValue };
      docs.push( objs );
   }
   return docs;
}

