/***************************************************************************************************
 * @Description: 主子表加密，子表各自开启Encrypted选项
 * @ATCaseID: encryption_3
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
 *    对数据库分区集合开启加密，验证其主子表的编目信息和记录数据是否符合预期
 * 测试步骤：
 *    1.创建主表，指定Encrypted:true，预期得到报错SDB_INVALIDARG
 *    2.创建主表，不可指定Encrypted:true，关闭压缩，并指定分区键和分区方式
 *    3.创建两个子表，子表1开启加密，子表2不开启加密，并挂载到主表上
 *    4.验证主表和两个子表的编目信息的Encrypted是否符合预期
 *    5.根据分区键，向两个子表插入、更新、删除记录，校验数据是否符合预期
 * 期望结果：
 *    编目信息和记录数据均符合预期
 **************************************************************************************************/
main(test);
function test() {
  var clName = "encryption_3";
  assert.tryThrow(SDB_OPTION_NOT_SUPPORT, function () {
    commCreateCL(db, COMMCSNAME, clName, {
      IsMainCL: true,
      ShardingType: "range",
      ShardingKey: { id: 1 },
      Encrypted: true,
    });
  });
  var cl = commCreateCL(db, COMMCSNAME, clName, {
    IsMainCL: true,
    ShardingType: "range",
    ShardingKey: { id: 1 },
  });
  var subCLName1 = clName + "_sub1";
  var subCLName2 = clName + "_sub2";
  commCreateCL(db, COMMCSNAME, subCLName1, { Encrypted: true });
  commCreateCL(db, COMMCSNAME, subCLName2, { Encrypted: false });
  cl.attachCL(COMMCSNAME + "." + subCLName1, {
    LowBound: { id: 0 },
    UpBound: { id: 100 },
  });
  cl.attachCL(COMMCSNAME + "." + subCLName2, {
    LowBound: { id: 100 },
    UpBound: { id: 200 },
  });
  checkUnencrypted(db, COMMCSNAME, clName);
  checkEncrypted(db, COMMCSNAME, subCLName1);
  checkUnencrypted(db, COMMCSNAME, subCLName2);

  // 插入
  var expRecs = [];
  for (var i = 0; i < 200; i++) {
    var record = { id: i, a: i };
    cl.insert(record);
    expRecs.push(record);
  }
  var cursor = cl.find().sort({ id: 1 });
  commCompareResults(cursor, expRecs);

  // 更新
  cl.update({ $set: { a: 1000 } }, { id: { $et: 5 } });
  cl.update({ $set: { a: 2000 } }, { id: { $et: 105 } });
  expRecs[5].a = 1000;
  expRecs[105].a = 2000;
  var cursor = cl.find().sort({ id: 1 });
  commCompareResults(cursor, expRecs);

  // 删除
  cl.remove({ id: 1 });
  cl.remove({ id: 101 });
  expRecs = expRecs.filter(function (obj) {
    return obj.id != 1 && obj.id != 101;
  });
  var cursor = cl.find().sort({ id: 1 });
  commCompareResults(cursor, expRecs);
}
