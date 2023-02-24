/***************************************************************************************************
 * @Description: 外部模式管理：重命名普通集合外部模式的字段
 * @ATCaseID: schema_25
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/23/2023 Zhou Hongye Rename schema column of common collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    创建普通集合，不开启InfoSchema，插入一定数据，开启InfoSchema，并绑定外部模式，重命名其字段
 *
 * 测试步骤：
 *    1.创建集合，且不开启内部模式，插入含有a,b字段的记录
 *    2.开启集合的内部模式，创建外部模式，其中a字段无默认值，b字段指定默认值，绑定模式，检查内部模式仅有b字段
 *    3.重命名外部模式a,b字段
 *    4.检查记录a,b字段名已修改为新的字段名
 *
 * 期望结果：
 *    所有校验均符合预期
 *
 **************************************************************************************************/
testConf.clName = "schema_25";
testConf.clOpt = { EnableInfoSchema: false };
main(test);

function test(testPara) {
  var clName = testConf.clName;
  var cl = testPara.testCL;
  cl.insert({ a: 1, b: 2 });
  cl.alter({ EnableInfoSchema: true });
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    a: { Type: "int32" },
    b: { Type: "int32", WriteDefault: 5, ReadDefault: 10 },
  });
  cl.addSchema(schemaName);
  // 内部模式中仅存在指定了默认值的字段b,而不存在字段a
  commCheckInternalSchemaHasNoColumn(
    db,
    COMMCSNAME,
    clName,
    {
      b: { WriteDefault: 5, ReadDefault: 10 },
      a: {},
    },
    "a"
  );
  schema.renameColumn("a", "new_a");
  schema.renameColumn("b", "new_b");
  commCheckInternalSchema(db, COMMCSNAME, clName, {
    new_a: {},
    new_b: { WriteDefault: 5, ReadDefault: 10 },
  });

  var expRecs = [{ new_a: 1, new_b: 2 }];
  var cursor = cl.find();
  commCompareResults(cursor, expRecs);
  commDropCL(db, COMMCSNAME, clName);
}
