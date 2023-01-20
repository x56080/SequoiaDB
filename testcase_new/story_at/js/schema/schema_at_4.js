/***************************************************************************************************
 * @Description: 外部模式管理：创建外部模式
 * @ATCaseID: schema_4
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Create External Schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.指定正确的参数进行创建
 *    2.创建与已经存在的外部模式同名的新外部模式
 *    3.指定错误的参数进行创建，包括参数个数、类型错误，字段信息错误
 *
 * 测试步骤：
 *    1.创建外部模式，并根据场景配置参数
 * 期望结果：
 *    场景1：创建成功
 *    场景2：报错误码SDB_CAT_INFOSCHEMA_EXIST
 *    场景3：报错误码SDB_INVALIDARG
 **************************************************************************************************/
testConf.clName = "schema_4";
testConf.clOpt = { EnableInfoSchema: true };
main(test);

function test() {
  // 1.指定正确的参数进行创建
  var schemaName1 = testConf.clName + "_1";
  commClearLegacySchema(db, schemaName1);
  db.createSchema(schemaName1, DEFAULT_SCHEMA_FIELDS_DEFINE);

  // 2.创建与已经存在的外部模式同名的新外部模式
  assert.tryThrow(SDB_SCHEMA_EXIST, function () {
    db.createSchema(schemaName1, DEFAULT_SCHEMA_FIELDS_DEFINE);
  });
  db.dropSchema(schemaName1);

  // 3.指定错误的参数进行创建，包括参数个数、类型错误，字段信息错误
  var schemaName2 = testConf.clName + "_2";
  commClearLegacySchema(db, schemaName2);
  // 参数个数错误
  assert.tryThrow(SDB_INVALIDARG, function () {
    db.createSchema(schemaName2);
  });
  // 字段类型错误
  assert.tryThrow(SDB_INVALIDARG, function () {
    db.createSchema(schemaName2, {
      Type: "wrongtype",
    });
  });
  // 字段信息错误
  assert.tryThrow(SDB_INVALIDARG, function () {
    db.createSchema(schemaName2, {
      field1: { Type: "string", WrongFieldInfo: 10 },
    });
  });
}
