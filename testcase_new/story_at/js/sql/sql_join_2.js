/***************************************************************************************************
 * @Description: filter of right join
 * @ATCaseID: sql_join_2
 * @Author: He Guoming
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 01/04/2023 He Guoming filter of right join
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    right join中左表的谓词条件不能下压
 * 测试步骤：
 *    1.right join中左表指定谓词条件
 *    1.right join中右表指定谓词条件
 * 期望结果：
 *    1.谓词条件能匹配正确
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_sql_join_2_a" ;

main( test ) ;

function test( testPara ) {
   var cl2name = COMMCLNAME + "_sql_join_2_b" ;
   cs = db.getCS( testConf.csName ) ;
   cl1 = cs.getCL( testConf.clName ) ;
   cl2 = cs.createCL( cl2name ) ;

   cl1.insert( {a:1, b:10} ) ;
   cl1.insert( {a:2, b:20} ) ;

   cl2.insert( {a:1, c:30} ) ;
   cl2.insert( {a:2, c:40} ) ;
   cl2.insert( {a:3, c:50} ) ;

   var res = db.exec( "select T1.a, T1.b, T2.c from " + testConf.csName + "." + testConf.clName +
                      " as T1 right outer join " + testConf.csName + "." + cl2name +
                      " as T2 on T1.a = T2.a where T1.a > 1" ) ;
   var expRes = [ { "a": 2, "b": 20, "c": 40 } ] ;
   commCompareResults( res, expRes ) ;

   var res = db.exec( "select T1.a, T1.b, T2.c from " + testConf.csName + "." + testConf.clName +
                      " as T1 right outer join " + testConf.csName + "." + cl2name +
                      " as T2 on T1.a = T2.a where T1.a < 2" ) ;
   var expRes = [ { "a": 1, "b": 10, "c": 30 } ] ;
   commCompareResults( res, expRes ) ;

   var res = db.exec( "select T1.a, T1.b, T2.c from " + testConf.csName + "." + testConf.clName +
                      " as T1 right outer join " + testConf.csName + "." + cl2name +
                      " as T2 on T1.a = T2.a where T2.a > 1" ) ;
   var expRes = [ { "a": 2, "b": 20, "c": 40 }, { "a": null, "b": null, "c": 50 } ] ;
   commCompareResults( res, expRes ) ;
}
