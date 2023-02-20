/***************************************************************************************************
 * @Description: 内部模式管理:写数据驱动内部模式更新
 * @ATCaseID: schema_22
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
 *    内部模式管理:写数据驱动内部模式更新
 * 测试步骤：
 *    1.创建集合，并绑定外部模式
 *    2.向集合插入一定数据，检查内部模式
 *    3.更新数据，增加内部模式中不存在的新字段，检查内部模式
 * 期望结果：
 *    插入的记录和更新的记录中含有的所有字段都添加到内部模式中
 *
 **************************************************************************************************/
testConf.clName = "schema_22"
testConf.clOpt = {EnableInfoSchema:true}
main(test);

function test() {
  var cl = testPara.testCL;
  var schemaName = testConf.clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5, WriteDefault: 10 },
    b: { Type: "string", ReadDefault: "default b" },
  });
  cl.addSchema(schemaName);
  cl.insert({c: 1.1});
  commCheckInternalSchema(db, COMMCSNAME, testConf.clName, {
    _id: {},
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { ReadDefault: "default b" },
    c: {},
  });
  cl.update({ $set: { d: 3.0 } }, { c: { $et: 1.1 } });
  commCheckInternalSchema(db, COMMCSNAME, testConf.clName, {
    _id: {},
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { ReadDefault: "default b" },
    c: {},
    d: {},
  });
}
