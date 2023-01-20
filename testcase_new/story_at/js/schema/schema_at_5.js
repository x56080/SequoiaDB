/***************************************************************************************************
 * @Description: 外部模式管理：删除外部模式
 * @ATCaseID: schema_5
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Drop External Schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.删除未与集合绑定的外部模式
 *    2.删除与集合绑定的外部模式
 * 测试步骤：
 *    1.创建外部模式，根据场景选择是否与集合绑定
 *    2.删除外部模式
 * 期望结果：
 *    1.删除成功
 *    2.不可删除，报SDB_OPERATION_INCOMPATIBLE
 **************************************************************************************************/
main(test);

function test() {
  var clname = "schema_5";
  // scene 1
  var schemaName1 = clname + "_1";
  commClearLegacySchema(db, schemaName1);
  db.createSchema(schemaName1, DEFAULT_SCHEMA_FIELDS_DEFINE);
  db.dropSchema(schemaName1);

  // scene 2
  var schemaName2 = clname + "_2";
  commClearLegacySchema(db, schemaName2);
  db.createSchema(schemaName2, DEFAULT_SCHEMA_FIELDS_DEFINE);
  var cl = commCreateCL(db, COMMCSNAME, clname, { EnableInfoSchema: true });
  cl.addSchema(schemaName2);
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    db.dropSchema(schemaName2);
  });
  commDropCL(db, COMMCSNAME, clname);
}
