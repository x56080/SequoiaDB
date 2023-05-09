/***************************************************************************************************
 * @Description: sdbexprt工具导出加密数据
 * @ATCaseID: encryption_5
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
 *    sdbexprt工具导出加密数据
 * 测试步骤：
 *    1.创建普通集合，开启加密
 *    2.插入数据
 *    3.使用sdbexprt工具分别导出csv和json，并校验导出内容
 * 期望结果：
 *    导出内容与记录数据匹配
 **************************************************************************************************/
testConf.clName = "encryption_5";
testConf.clOpt = { Encrypted: true, Compressed: false };
main(test);
function test(testPara) {
  var cl = testPara.testCL;
  var expRecs = [];
  for (var i = 0; i < 10; i++) {
    cl.insert({ _id: i, a: i });
    expRecs.push({ _id: i, a: i });
  }

  var cmd = new Cmd();
  var tmpFileDir = WORKDIR + "sdbexprt/";
  cmd.run("rm -rf " + tmpFileDir);
  cmd.run("mkdir -p " + tmpFileDir);

  var csvFile = WORKDIR + "sdbexprt/" + testConf.clName + ".csv";
  cmd.run("rm -rf " + csvFile);

  var command =
    installPath +
    "bin/sdbexprt" +
    " -s " +
    COORDHOSTNAME +
    " -p " +
    COORDSVCNAME +
    " -c " +
    COMMCSNAME +
    " -l " +
    testConf.clName +
    " --file " +
    csvFile +
    " --type csv" +
    " --fields _id,a" +
    " --withid true ";

  cmd.run(command);
  checkCsvFileContent(csvFile, expRecs, ["_id", "a"]);
  cmd.run("rm -rf " + csvFile);

  var jsonFile = WORKDIR + "sdbexprt/" + testConf.clName + ".json";
  cmd.run("rm -rf " + jsonFile);

  var command =
    installPath +
    "bin/sdbexprt" +
    " -s " +
    COORDHOSTNAME +
    " -p " +
    COORDSVCNAME +
    " -c " +
    COMMCSNAME +
    " -l " +
    testConf.clName +
    " --file " +
    jsonFile +
    " --type json";

  cmd.run(command);
  checkJsonFileContent(jsonFile, expRecs);
  cmd.run("rm -rf " + jsonFile);
}
