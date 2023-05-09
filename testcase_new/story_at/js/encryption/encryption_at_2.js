/***************************************************************************************************
 * @Description: 数据库分区集合加密
 * @ATCaseID: encryption_
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
 *    对数据库分区集合开启加密，验证其编目信息和记录数据是否符合预期
 * 测试步骤：
 *    1.创建数据库分区集合，开启加密、关闭压缩，并指定分区键和分区方式
 *    2.分区到另一个复制组，验证该集合的编目信息是否开启Encrypted
 *    3.插入、更新、删除记录数据并校验数据是否符合预期
 * 期望结果：
 *    编目信息和记录数据均符合预期
 **************************************************************************************************/
main(test);
function test(testPara) {
  var groupNames = commGetDataGroupNames(db);
  assert.equal(groupNames.length > 1, true);

  var clName = "encryption_2";
  var cl = commCreateCL(db, COMMCSNAME, clName, {
    Encrypted: true,
    Compressed: false,
    ShardingKey: { id: 1 },
    ShardingType: "hash",
    Group: groupNames[0],
  });
  cl.split(groupNames[0], groupNames[1], { id: 2048 }, { id: 4096 });
  checkEncrypted(db, COMMCSNAME, clName);
  var expRecs = [];
  // 插入
  for (var i = 0; i < 10; i++) {
    var record = { id: i, a: i, b: i };
    cl.insert(record);
    expRecs.push(record);
  }
  var cursor = cl.find().sort({ id: 1 });
  commCompareResults(cursor, expRecs);

  // 更新
  cl.update({ $set: { a: 20 } }, { a: 5 });
  expRecs[5].a = 20;
  var cursor = cl.find().sort({ id: 1 });
  commCompareResults(cursor, expRecs);

  // 删除
  cl.remove({ b: 1 });
  expRecs.splice(1, 1);
  var cursor = cl.find().sort({ id: 1 });
  commCompareResults(cursor, expRecs);
}
