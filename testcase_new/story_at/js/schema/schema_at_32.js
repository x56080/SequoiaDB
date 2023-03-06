/***************************************************************************************************
 * @Description: 在绑定模式和开启数据压缩情况下，校验集合尾部数据
 * @ATCaseID: schema_32
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 03/02/2023 Zhou Hongye Commit and rollback transaction on a collection bound to schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.
 *
 * 测试步骤：
 *    1.
 *
 * 期望结果：
 *
 *
 **************************************************************************************************/
testConf.clName = "schema_32";
testConf.clOpt = { EnableInfoSchema: true, Compressed: true };
main(test);

function test(testPara) {
  var cl = testPara.testCL;
  var clName = testConf.clName;
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5, WriteDefault: 10 },
    b: { Type: "string", ReadDefault: "read default b", WriteDefault: "write default b" },
    c: { Type: "double", ReadDefault: 5.5, WriteDefault: 10.5 },
  });
  cl.createIndex("index_pos", { pos: -1 });
  cl.addSchema(schemaName);
  var pad = "b";
  for (var i = 0; i < 700; ++i) {
    pad += "b";
  }
  var records = [];
  var expRecs = [];
  for (var i = 0; i < 200000; ++i) {
    var r = { pos: i, a: 1, b: pad, c: 2.5 };
    records.push(r);
    if (i % 10000 == 0) expRecs.push(r);
  }
  cl.insert(records);
  expRecs.reverse();
  println(expRecs.length);
  var cursor = cl.find({ pos: { $mod: [10000, 0] } }).hint({ "": "index_pos" });
  commCompareResults(cursor, expRecs);
}
