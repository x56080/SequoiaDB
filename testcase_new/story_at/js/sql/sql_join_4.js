/***************************************************************************************************
 * @Description: filter of left join
 * @ATCaseID: sql_join_4
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
 *    多表left join中的谓词条件可以下压
 * 测试步骤：
 *    1.多表left join中左表指定谓词条件
 *    1.多表left join中右表指定谓词条件
 * 期望结果：
 *    1.谓词条件能匹配正确
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_sql_join_4_a" ;

main( test ) ;

function test( testPara ) {
   var cl2name = COMMCLNAME + "_sql_join_4_b" ;
   var cl3name = COMMCLNAME + "_sql_join_4_c" ;
   cs = db.getCS( testConf.csName ) ;
   cl1 = cs.getCL( testConf.clName ) ;
   cl2 = cs.createCL( cl2name ) ;
   cl3 = cs.createCL( cl3name ) ;

   cl1.insert( {a:1, b:10} ) ;
   cl1.insert( {a:2, b:20} ) ;
   cl1.insert( {a:3, b:30} ) ;
   cl1.insert( {a:4, b:40} ) ;

   cl2.insert( {a:1, c:30} ) ;
   cl2.insert( {a:2, c:40} ) ;
   cl2.insert( {a:3, c:50} ) ;

   cl3.insert( {a:1, d:40} ) ;
   cl3.insert( {a:2, d:50} ) ;

   var res = db.exec( "select T1.a, T1.b, T4.c, T4.d, T4.a as e, T4.b as f from " + testConf.csName + "." + testConf.clName +
                      " as T1 left outer join " +
                      "( select T2.a as a, T3.a as b, T2.c, T3.d from " + testConf.csName + "." + cl2name +
                      " as T2 left outer join " + testConf.csName + "." + cl3name + " as T3 on T2.a = T3.a where T3.a > 1 ) " +
                      " as T4 on T1.a = T4.a where T4.a > 1" ) ;
   var expRes = [ { "a": 2, "b": 20, "c": 40, "d": 50, "e": 2, "f": 2 } ] ;
   commCompareResults( res, expRes ) ;

   var res = db.exec( "select T1.a, T1.b, T4.c, T4.d, T4.a as e, T4.b as f from " + testConf.csName + "." + testConf.clName +
                      " as T1 left outer join " +
                      "( select T2.a as a, T3.a as b, T2.c, T3.d from " + testConf.csName + "." + cl2name +
                      " as T2 left outer join " + testConf.csName + "." + cl3name + " as T3 on T2.a = T3.a ) " +
                      " as T4 on T1.a = T4.a where T4.a > 1" ) ;
   var expRes = [ { "a": 2, "b": 20, "c": 40, "d": 50, "e": 2, "f": 2 }, { "a": 3, "b": 30, "c": 50, "d": null, "e": 3, "f": null } ] ;
   commCompareResults( res, expRes ) ;
}
