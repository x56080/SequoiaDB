/***************************************************************************************************
 * @Description: 内部模式管理:truncate集合时内部模式的清理
 * @ATCaseID: schema_21
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/17/2023 Zhou Hongye Clear internal schema when truncate
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.未绑定外部模式的集合进行truncate
 *    2.绑定了外部模式的集合进行truncate
 * 测试步骤：
 *    1.创建集合，并绑定外部模式
 *    2.向集合插入一定数据，检查内部模式
 *    3.执行truncate后，检查内部模式
 * 期望结果：
 *    1.内部模式清空
 *    2.内部模式中只包含外部模式中定义了默认值的字段
 *
 **************************************************************************************************/
testConf.clName = "schema_21"
testConf.clOpt = {EnableInfoSchema:true}
main(test);

function test() {
  var cl = testPara.testCL;
  cl.insert({a: 1});
  checkInternalSchema(db, COMMCSNAME, testConf.clName, { _id: {}, a: {} });
  cl.insert({b: "1"});
  checkInternalSchema(db, COMMCSNAME, testConf.clName, { _id: {}, a: {}, b:{} });
  cl.truncate();
  checkInternalSchema(db, COMMCSNAME, testConf.clName, {});

  var schemaName = testConf.clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5, WriteDefault: 10 },
    b: { Type: "string", ReadDefault: "default b" },
  });
  cl.addSchema(schemaName);
  cl.insert({c: 1.1});
  checkInternalSchema(db, COMMCSNAME, testConf.clName, {
    _id: {},
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { ReadDefault: "default b" },
    c: {},
  });
  cl.truncate();
  checkInternalSchema(db, COMMCSNAME, testConf.clName, {
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { ReadDefault: "default b" },
  });
}
