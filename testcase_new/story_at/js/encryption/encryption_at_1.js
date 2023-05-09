/***************************************************************************************************
 * @Description: 普通集合加密
 * @ATCaseID: encryption_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 03/22/2023 Zhou Hongye    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    对普通集合开启加密，验证其编目信息和记录数据是否符合预期
 * 测试步骤：
 *    1.创建普通集合，开启加密、关闭压缩
 *    2.验证该集合的编目信息是否开启Encrypted
 *    3.插入、更新、删除记录数据并校验数据是否符合预期
 * 期望结果：
 *    编目信息和记录数据均符合预期
 **************************************************************************************************/
testConf.clName = "encryption_1";
testConf.clOpt = { Encrypted: true, Compressed: false };
main(test);
function test(testPara) {
  var cl = testPara.testCL;
  var clName = testConf.clName;
  checkEncrypted(db, COMMCSNAME, clName);
  var expRecs = [];
  // 插入
  for (var i = 0; i < 10; i++) {
    var record = { a: i, b: i };
    cl.insert(record);
    expRecs.push(record);
  }
  var cursor = cl.find();
  commCompareResults(cursor, expRecs);

  // 更新
  cl.update({ $set: { a: 20 } }, { a: 5 });
  expRecs[5].a = 20;
  var cursor = cl.find();
  commCompareResults(cursor, expRecs);

  // 删除
  cl.remove({ b: 1 });
  expRecs.splice(1, 1);
  var cursor = cl.find();
  commCompareResults(cursor, expRecs);
}
