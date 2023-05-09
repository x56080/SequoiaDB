/***************************************************************************************************
 * @Description: 普通集合overflow记录数据
 * @ATCaseID: encryption_7
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 04/17/2023 Zhou Hongye    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境
 * 测试场景：
 *     加密集合overflow记录数据的增删改查
 * 测试步骤：
 *     1.创建开启加密的集合
 *     2.插入一定量数据，记录长度为1000字节左右
 *     3.更新其中一条记录，更新其记录长度到2000字节左右，预期其会成为一条overflow的记录
 *     
 *
 * 期望结果：
 *     overflow记录数据符合预期
 *
 **************************************************************************************************/

testConf.clName = "encryption_7";
testConf.clOpt = { Encrypted: true };

main(test);
function test(testPara) {
  var cl = testPara.testCL;
  var text = "";
  for (var i = 0; i < 1024; i++) {
    text += "t";
  }

  expRecs = [];
  for (var i = 0; i < 64 * 1024; i++) {
    var record = { id: i, pad: text }
    cl.insert(record);
    expRecs.push(record);
  }
  for (var i = 0; i < 1024; i++) {
    text += "t";
  }
  cl.update({ $set: { pad: text } }, { id: 1 });
  expRecs[1] = { id: 1, pad: text };

  var cursor = cl.find();
  commCompareResults(cursor, expRecs);
}
