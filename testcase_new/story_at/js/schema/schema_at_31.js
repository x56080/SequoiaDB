/***************************************************************************************************
 * @Description: 对绑定了外部模式的集合做事务提交和回滚(隔离级别RC)
 * @ATCaseID: schema_31
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/28/2023 Zhou Hongye Commit and rollback transaction on a collection bound to schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可，开启事务，隔离级别为RC
 * 测试场景：
 *    1.创建集合并绑定外部模式
 *    2.开启事务，插入数据，提交、回滚事务，检查集合的内部模式和记录数据
 *    3.开启事务，更新数据，提交、回滚事务，检查集合的内部模式和记录数据
 *    4.开启事务，删除数据，提交、回滚事务，检查集合的内部模式和记录数据
 *
 * 测试步骤：
 *    1.
 *
 * 期望结果：
 *
 *
 **************************************************************************************************/
testConf.clName = "schema_31";
testConf.clOpt = { EnableInfoSchema: true };
main(test);

function test(testPara) {
  // 设置隔离级别为rc
  db.updateConf({transisolation: 1});
  var clName = testConf.clName;
  var cl = testPara.testCL;

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32" },
    b: { Type: "int32", ReadDefault: 5 },
    c: { Type: "double", ReadDefault: 5.5, WriteDefault: 10.5 },
  });
  cl.addSchema(schemaName);

  checkInternalSchemaHasNoColumn(
    db,
    COMMCSNAME,
    clName,
    {
      a: {},
      b: { ReadDefault: 5 },
      c: { ReadDefault: 5.5, WriteDefault: 10.5 },
    },
    "a"
  );

  // 对触发了内部模式进化的插入记录进行回滚，不会回滚内部模式
  db.transBegin();
  cl.insert({ a: 1 });
  checkInternalSchema(db, COMMCSNAME, clName, {
    a: {},
    b: { ReadDefault: 5 },
    c: { ReadDefault: 5.5, WriteDefault: 10.5 },
  });
  db.transRollback();
  checkInternalSchema(db, COMMCSNAME, clName, {
    a: {},
    b: { ReadDefault: 5 },
    c: { ReadDefault: 5.5, WriteDefault: 10.5 },
  });
  assert.equal(cl.count(), 0);

  // 事务提交插入非贴源记录
  db.transBegin();
  cl.insert({ a: 1 });
  db.transCommit();
  checkInternalSchema(db, COMMCSNAME, clName, {
    a: {},
    b: { ReadDefault: 5 },
    c: { ReadDefault: 5.5, WriteDefault: 10.5 },
  });
  var cursor = cl.find().flags(SDB_FLG_QUERY_PRIMAL_DATA);
  commCompareResults(cursor, [{ a: 1, c: 10.5 }]);
  var cursor = cl.find();
  commCompareResults(cursor, [{ a: 1, b: 5, c: 10.5 }]);

  // 事务回滚更新非贴源记录
  cl.insert({ a: 2 });
  db.transBegin();
  cl.update({ $set: { c: 11.5 } }, { a: 2 });
  var expRecord = { a: 2, b: 5, c: 11.5 };
  var cursor = cl.find({ a: 2 });
  commCompareResults(cursor, [expRecord]);
  db.transRollback();
  var cursor = cl.find({ a: 2 });
  commCompareResults(cursor, [{ a: 2, b: 5, c: 10.5 }]);

  // 事务提交更新非贴源记录
  cl.insert({ a: 3 });
  db.transBegin();
  cl.update({ $set: { c: 11.5 } }, { a: 3 });
  var expRecord = { a: 3, b: 5, c: 11.5 };
  var cursor = cl.find({ a: 3 });
  commCompareResults(cursor, [expRecord]);
  db.transCommit();
  var cursor = cl.find({ a: 3 }).flags(SDB_FLG_QUERY_PRIMAL_DATA);
  commCompareResults(cursor, [expRecord]);

  // 事务回滚删除非贴源记录
  cl.insert({ a: 4 });
  db.transBegin();
  cl.remove({ a: 4 });
  assert.equal(cl.find({ a: 4 }).size(), 0);
  db.transRollback();
  var cursor = cl.find({ a: 4 }).flags(SDB_FLG_QUERY_PRIMAL_DATA);
  commCompareResults(cursor, [{ a: 4, c: 10.5 }]);
  var cursor = cl.find({ a: 4 });
  commCompareResults(cursor, [{ a: 4, b: 5, c: 10.5 }]);

  // 事务提交删除非贴源记录
  db.transBegin();
  cl.remove({ a: 4 });
  db.transCommit();
  assert.equal(cl.find({ a: 4 }).size(), 0);

  // 隔离级别rc下可避免脏读，在一个事务中不会读取到另一个事务未提交的对非贴源记录的更新
  var conn1 = db;
  var conn2 = Sdb(COORDHOSTNAME, COORDSVCNAME, REMOTEUSER, REMOTEPASSWD);
  var cl1 = conn1.getCS(COMMCSNAME).getCL(clName);
  var cl2 = conn2.getCS(COMMCSNAME).getCL(clName);
  cl1.insert({ a: 5, c: 12.0 });
  conn1.setSessionAttr({TransLockWait: false, TransUseRBS: true});
  conn1.transBegin();
  conn2.transBegin();
  cl2.update({ $set: { b: 10, c: 13.0 } }, { a: 5 });
  var cursor = cl1.find({a:5});
  commCompareResults(cursor, [{ a: 5, b: 5, c: 12.0 }]);
  conn2.transCommit();
  conn1.transCommit();

  commDropCL(db, COMMCSNAME, clName);
}
