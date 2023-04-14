/***************************************************************************************************
 * @Description: mixcmp in SQL
 * @ATCaseID: sql_mixcmp_2
 * @Author: He Guoming
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 01/04/2023 He Guoming mixcmp in SQL
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    mixcmp在SQL场景中使用
 * 测试步骤：
 *    1.在集合中插入a字段为字符串的记录
 *    2.在SQL中通过>和>=对a字段进行数值比较过滤
 * 期望结果：
 *    1.字符串的记录不能匹配
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_sql_mix_3" ;

main( test ) ;

function test( testPara ) {
   cs = db.getCS( testConf.csName ) ;
   cl1 = cs.getCL( testConf.clName ) ;

   cl1.insert({a:1,b:1})
   cl1.insert({b:2})
   cl1.insert({a:"a",b:3})

   var res = db.exec( "select * from ( select first(a) as a from " +
                      testConf.csName + "." + testConf.clName +
                      " group by b ) as T where T.a > 0" ) ;
   var expRes = [ { "a": 1 } ] ;
   commCompareResults( res, expRes ) ;

   var res = db.exec( "select * from ( select first(a) as a from " +
   testConf.csName + "." + testConf.clName +
   " group by b ) as T where T.a >= 0" ) ;
   var expRes = [ { "a": 1 } ] ;
   commCompareResults( res, expRes ) ;
}
