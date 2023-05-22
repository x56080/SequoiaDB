/***************************************************************************************************
 * @Description: 加密集合在RC隔离级别事务的更新和查询
 * @ATCaseID: encryption_8
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 05/19/2023 Zhou Hongye    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境
 * 测试场景：
 *     RC隔离级别事务
 * 测试步骤：
 *     1.创建开启加密的集合，并插入一条记录
 *     2.设置会话隔离级别
 *     3.事务1更新该条记录，不提交
 *     4.事务2查询该条记录，预期得到更新前的记录
 *     5.提交事务2和事务1
 *     6.再次查询该条记录，预期得到更新后的记录
 *
 * 期望结果：
 *     查询到的记录符合RC隔离级别的预期 
 *
 **************************************************************************************************/

testConf.clName = "encryption_8";
testConf.clOpt = { Encrypted: true };

main(test);
function test(testPara) {
  var conn = Sdb(COORDHOSTNAME, COORDSVCNAME);
  db.setSessionAttr({ TransIsolation: 1, TransLockWait: false });
  conn.setSessionAttr({ TransIsolation: 1, TransLockWait: false });

  var record = { id: 1, k: 1 };
  var cl = testPara.testCL;
  cl.insert(record);

  db.transBegin();
  cl.update({ $set: { k: 2 } }, { id: 1 });

  cl2 = conn.getCS(testConf.csName).getCL(testConf.clName);

  conn.transBegin();
  var cursor = cl2.find({ id: 1 });
  commCompareResults(cursor, [record]);
  conn.transCommit();

  db.transCommit();

  var cursor = cl2.find({ id: 1 });
  commCompareResults(cursor, [{ id: 1, k: 2 }]);
}
