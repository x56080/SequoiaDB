/***************************************************************************************************
 * @Description: 加密集合LOB数据写入、截断、读取、列举
 * @ATCaseID: encryption_6
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 04/13/2023 Zhou Hongye    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境
 * 测试场景：
 *     加密集合LOB数据写入读取
 * 测试步骤：
 *     1.创建开启加密的集合
 *     2.插入LOB并读取，校验数据是否正确
 *     3.插入LOB截断后读取，校验数据是否正确
 *     4.列出所有LOB，校验每个LOB的长度是否符合预期
 * 期望结果：
 *     读取的LOB数据和元数据中记录的长度符合预期
 *
 **************************************************************************************************/

testConf.clName = "encryption_6";
testConf.clOpt = { Encrypted: true };

main(test);
function test(testPara) {
  var cl = testPara.testCL;
  var data = "lob data content";
  var oidToLength = {};
  var oid = checkPutLob(cl, data);
  oidToLength[oid] = data.length;
  var oid = checkTruncateLob(cl, data, 5);
  oidToLength[oid] = 5;

  var longdata = "";
  for (var i = 0; i < 60000; i++) {
    // total length = 10 * 60000
    longdata += "abcdefghij";
  }
  var oid = checkPutLob(cl, longdata);
  oidToLength[oid] = longdata.length;
  var oid = checkTruncateLob(cl, longdata, 400000);
  oidToLength[oid] = 400000;

  checkListLobs(cl, oidToLength);
}

function checkPutLob(cl, data) {
  var lobFilePath = WORKDIR + testConf.clName + ".txt";
  var file = File(lobFilePath);
  file.truncate();
  file.write(data);
  file.close();
  var oid = cl.putLob(lobFilePath);
  var readLobFilePath = WORKDIR + testConf.clName + "_read.txt";
  cl.getLob(oid, readLobFilePath, true);
  checkFileContent(readLobFilePath, data);
  return oid;
}

function checkTruncateLob(cl, data, length) {
  var lobFilePath = WORKDIR + testConf.clName + ".txt";
  var file = File(lobFilePath);
  file.truncate();
  file.write(data);
  file.close();
  var oid = cl.putLob(lobFilePath);
  var readLobFilePath = WORKDIR + testConf.clName + "_read.txt";
  cl.truncateLob(oid, length);
  cl.getLob(oid, readLobFilePath, true);
  checkFileContent(readLobFilePath, data.slice(0, length));
  return oid;
}

function checkListLobs(cl, expLobsLength) {
  var cursor = cl.listLobs();
  while (cursor.next()) {
    var meta = cursor.current().toObj();
    var oid = meta["Oid"]["$oid"];
    assert.equal(expLobsLength[oid], meta["Size"]);
  }
}
