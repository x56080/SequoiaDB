/***************************************************************************************************
 * @Description: 外部模式使用：绑定外部模式后对数据进行查询、更新、删除操作
 * @ATCaseID: schema_30
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/28/2023 Zhou Hongye Query, update and delete data on a collection bound to schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    查询：
 *    1.匹配非贴源记录中贴源字段，贴源字段排序
 *    2.匹配非贴源记录中贴源字段，非贴源字段排序
 *    3.匹配非贴源记录中非贴源字段，贴源字段排序
 *    更新：
 *    4.匹配非贴源记录中非贴源字段，更新非贴源字段
 *    5.匹配非贴源记录中非贴源字段，更新贴源字段
 *    6.匹配非贴源记录中贴源字段，更新非贴源字段
 *    7.匹配非贴源记录中贴源字段，更新贴源字段
 *    删除：
 *    8.匹配非贴源记录中非贴源字段，删除记录
 *    9.匹配非贴源记录中贴源字段，删除记录
 *
 *
 * 测试步骤：
 *    1.创建集合，预先插入数据并绑定外部模式，准备非贴源记录数据
 *    2.根据测试场景查询、更新、删除记录，并校验数据是否符合预期
 *
 * 期望结果：
 *    数据变化符合预期，更新后记录变为贴源记录。更新后校验主备数据节点一致
 *
 **************************************************************************************************/
main(test);

function test(testPara) {
  var clName = "schema_30";
  function queryTest1(cl) {
    var expRecs = [];
    for (var i = 2; i >= 0; i--) {
      expRecs.push({ rid: i, a: 5, b: i + 0.1 });
    }
    var cursor = cl.find({ rid: { $gte: 0, $lt: 3 } }).sort({ rid: -1 });
    commCompareResults(cursor, expRecs);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, queryTest1);

  function queryTest2(cl) {
    var expRecs = [];
    for (var i = 0; i < 3; i++) {
      expRecs.push({ rid: i, a: 5, b: i + 0.1 });
    }
    var cursor = cl.find({ rid: { $gte: 0, $lt: 3 } }).sort({ a: 1 });
    commCompareResults(cursor, expRecs.slice(0, 3));
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, queryTest2);

  function queryTest3(cl) {
    var expRecs = [];
    for (var i = 0; i < 5; i++) {
      expRecs.push({ rid: i, a: 5, b: i + 0.1 });
    }
    var cursor = cl.find({ a: 5 }).sort({ rid: 1 });
    commCompareResults(cursor, expRecs);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, queryTest3);

  function updateTest1(cl) {
    cl.update({ $inc: { a: 1 } }, { a: 5 });
    var expRecs = [];
    for (var i = 0; i < 5; i++) {
      expRecs.push({ rid: i, a: 6, b: i + 0.1 });
    }
    var cursor = cl.find({ a: 6 });
    commCompareResults(cursor, expRecs);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, updateTest1);

  function updateTest2(cl) {
    cl.update({ $set: { b: 6.0 } }, { a: 5 });
    var expRecs = [];
    for (var i = 0; i < 5; i++) {
      expRecs.push({ rid: i, a: 5, b: 6.0 });
    }
    var cursor = cl.find({ a: 5 });
    commCompareResults(cursor, expRecs);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, updateTest2);

  function updateTest3(cl) {
    cl.update({ $set: { a: 6 } }, { rid: 0 });
    var cursor = cl.find({ rid: 0 });
    commCompareResults(cursor, [{ rid: 0, a: 6, b: 0.1 }]);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, updateTest3);

  function updateTest4(cl) {
    cl.update({ $set: { b: 0.5 } }, { rid: 0 });
    var cursor = cl.find({ rid: 0 });
    commCompareResults(cursor, [{ rid: 0, a: 5, b: 0.5 }]);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, updateTest4);

  function removeTest1(cl) {
    cl.remove({ a: 5 });
    var cursor = cl.find({ a: 5 });
    commCompareResults(cursor, []);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, removeTest1);

  function removeTest2(cl) {
    cl.remove({ rid: 0 });
    var cursor = cl.find({ a: 0 });
    commCompareResults(cursor, []);
  }
  createCLWithPrimalDataAndTest(db, COMMCSNAME, clName, removeTest2);
}

function createCLWithPrimalDataAndTest(db, csName, clName, func) {
  var cl = commCreateCL(db, csName, clName, { EnableInfoSchema: true });
  var schemaName = clName + "_schema";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    rid: { Type: "int32" },
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double" },
  });
  for (var i = 0; i < 5; i++) {
    cl.insert({ rid: i, b: i + 0.1 });
  }
  cl.addSchema(schemaName);
  func(cl);
  commDropCL(db, csName, clName);
}
