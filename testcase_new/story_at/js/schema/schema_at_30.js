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
testConf.clName = "schema_30";
testConf.clOpt = { EnableInfoSchema: true };
main(test);

function test(testPara) {
  var clName = testConf.clName;
  var cl = testPara.testCL;

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, { rid: { Type: "int32" }, a: { Type: "int32", ReadDefault: 5 } });
  cl.addSchema(schemaName);

  var expRecs = [];
  var i = 0;
  for (; i < 5; i++) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i, a: 5 });
  }

  var cursor = cl.find({ rid: { $gte: 0, $lt: 3 } }).sort({ rid: -1 });
  commCompareResults(
    cursor,
    expRecs.slice(0, 3).sort(function (left, right) {
      return -(left.rid - right.rid);
    })
  );

  var cursor = cl.find({ rid: { $gte: 0, $lt: 3 } }).sort({ a: 1 });
  commCompareResults(cursor, expRecs.slice(0, 3));

  var cursor = cl.find({ a: 5 }).sort({ rid: -1 });
  commCompareResults(
    cursor,
    expRecs.slice(0, 5).sort(function (left, right) {
      return -(left.rid - right.rid);
    })
  );

  cl.update({ $inc: { a: 1 } }, { a: 5 });
  var cursor = cl.find().sort({ rid: 1 });
  expRecs = expRecs.map(function (v) {
    v.a += 1;
    return v;
  });
  commCompareResults(cursor, expRecs);
  checkConsistence(db, COMMCSNAME, clName, { rid: 1 }, expRecs, expRecs, {
    rid: {},
    a: { ReadDefault: 5 },
  });

  for (; i < 10; ++i) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i + 1, a: 5 });
  }
  cl.update({ $inc: { rid: 1 } }, { a: 5 });
  var cursor = cl.find({ a: 5 }).sort({ rid: 1 });
  commCompareResults(
    cursor,
    expRecs.filter(function (v) {
      return v.a == 5;
    })
  );
  checkConsistence(db, COMMCSNAME, clName, { rid: 1 }, expRecs, expRecs, {
    rid: {},
    a: { ReadDefault: 5 },
  });

  for (; i < 15; ++i) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i, a: 5 });
  }
  cl.update({ $set: { a: 6 } }, { rid: 14 });
  expRecs[14].a = 6;
  var cursor = cl.find().sort({ rid: 1 });
  commCompareResults(cursor, expRecs);
  expPrimalRecs = expRecs.concat();
  for (var j = 10; j < 14; ++j) {
    expPrimalRecs[j] = { rid: j };
  }
  checkConsistence(db, COMMCSNAME, clName, { rid: 1 }, expRecs, expPrimalRecs, {
    rid: {},
    a: { ReadDefault: 5 },
  });

  for (; i < 20; ++i) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i, a: 5 });
    expPrimalRecs.push({rid:i});
  }
  cl.update({ $set: { rid: 20 } }, { rid: 19 });
  expRecs[19].rid = 20;
  expPrimalRecs[19] = { rid: 20, a: 5 };
  var cursor = cl.find().sort({ rid: 1 });
  commCompareResults(cursor, expRecs);
  checkConsistence(db, COMMCSNAME, clName, { rid: 1 }, expRecs, expPrimalRecs, {
    rid: {},
    a: { ReadDefault: 5 },
  });

  cl.truncate();
  expRecs = [];
  for (i = 0; i < 5; ++i) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i, a: 5 });
  }
  cl.remove({ rid: { $gte: 0, $lt: 2 } });
  expRecs = expRecs.slice(2, 5);
  var cursor = cl.find().sort({ rid: 1 });
  commCompareResults(cursor, expRecs);
  cl.remove({ a: 5 });
  expRecs = [];
  var cursor = cl.find().sort({ rid: 1 });
  commCompareResults(cursor, expRecs);

  commDropCL(db, COMMCSNAME, clName);
}
