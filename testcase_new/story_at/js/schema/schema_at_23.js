/***************************************************************************************************
 * @Description: sdbexprt导出贴源数据
 * @ATCaseID: schema_23
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/17/2023 Zhou Hongye Use sdbexprt to export records
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.sdbexprt导出csv数据
 *    2.sdbexprt导出json数据
 * 测试步骤：
 *    1.创建集合并开启内部模式，绑定外部模式
 *    2.向集合插入一定数据
 *    3.变更外部模式，增加部分带默认值的字段
 *    4.使用sdbexprt导出数据
 * 期望结果：
 *    插入的记录和更新的记录中含有的所有字段都添加到内部模式中
 *
 **************************************************************************************************/
testConf.clName = "schema_23"
testConf.clOpt = {EnableInfoSchema:true}
main(test);

function test() {
  var cl = testPara.testCL;
  var schemaName = testConf.clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    _id: { Type: "int32" },
  });
  cl.addSchema(schemaName);
  var expRecs = [];
  var primalExpRecs = [];
  var i = 0;
  for( ;i < 50; ++i)
  {
    cl.insert({ _id: i });
    expRecs.push({ _id: i });
    primalExpRecs.push({ _id: i });
  }
  schema.addColumn("c", { Type: "string", ReadDefault: "read default c" });
  schema.addColumn("d", { Type: "string", WriteDefault: "write default d" });
  for( i = 0 ; i< 50; ++i )
  {
    expRecs[i]["c"] = "read default c";
  }
  for ( i = 50 ; i < 100; ++i )
  {
    cl.insert( { _id: i } );
    expRecs.push({ _id: i, d: "write default d" });
    primalExpRecs.push({ _id: i, d: "write default d" });
  }

  var cursor = cl.find();
  commCompareResults(cursor, expRecs, false);

  var cmd = new Cmd();
  var tmpFileDir = WORKDIR + "sdbexprt/";
  cmd.run( "rm -rf " + tmpFileDir );

  cmd.run( "mkdir -p " + tmpFileDir );
  
  // 导出贴源数据到csv
  var csvFile = WORKDIR + "sdbexprt/" + testConf.clName + "_primal.csv";
  cmd.run( "rm -rf " + csvFile );

  var command = installPath + "bin/sdbexprt" +
  " -s " + COORDHOSTNAME +
  " -p " + COORDSVCNAME +
  " -c " + COMMCSNAME +
  " -l " + testConf.clName +
  " --file " + csvFile +
  " --type csv" +
  " --fields _id,c,d" +
  " --withid true " +
  " --primal";


  cmd.run(command);
  checkCsvFileContent(csvFile, primalExpRecs, ["_id", "c", "d"]);
  cmd.run( "rm -rf " + csvFile );

  // 导出非贴源数据到csv
  var csvFile = WORKDIR + "sdbexprt/" + testConf.clName + ".csv";
  var cmd = new Cmd();
  cmd.run( "rm -rf " + csvFile );

  var command = installPath + "bin/sdbexprt" +
  " -s " + COORDHOSTNAME +
  " -p " + COORDSVCNAME +
  " -c " + COMMCSNAME +
  " -l " + testConf.clName +
  " --file " + csvFile +
  " --type csv" +
  " --fields _id,c,d" +
  " --withid true ";


  cmd.run(command);
  checkCsvFileContent(csvFile, expRecs, ["_id", "c", "d"]);
  cmd.run( "rm -rf " + csvFile );

  // 导出贴源数据到json
  var jsonFile = WORKDIR + "sdbexprt/" + testConf.clName + "_primal.json";
  var cmd = new Cmd();
  cmd.run( "rm -rf " + jsonFile );

  var command = installPath + "bin/sdbexprt" +
  " -s " + COORDHOSTNAME +
  " -p " + COORDSVCNAME +
  " -c " + COMMCSNAME +
  " -l " + testConf.clName +
  " --file " + jsonFile +
  " --type json" +
  " --primal ";

  cmd.run(command);
  checkJsonFileContent(jsonFile, primalExpRecs);
  cmd.run( "rm -rf " + jsonFile );

  // 导出非贴源数据到json
  var jsonFile = WORKDIR + "sdbexprt/" + testConf.clName + ".json";
  var cmd = new Cmd();
  cmd.run( "rm -rf " + jsonFile );

  var command = installPath + "bin/sdbexprt" +
  " -s " + COORDHOSTNAME +
  " -p " + COORDSVCNAME +
  " -c " + COMMCSNAME +
  " -l " + testConf.clName +
  " --file " + jsonFile +
  " --type json";

  cmd.run(command);
  checkJsonFileContent(jsonFile, expRecs);
  cmd.run( "rm -rf " + jsonFile );
}
