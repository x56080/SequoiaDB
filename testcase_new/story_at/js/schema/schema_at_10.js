/***************************************************************************************************
 * @Description: 外部模式使用：给普通集合绑定外部模式
 * @ATCaseID: schema_10
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Bind external schema to a common collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.普通集合未开启内部模式的情况下给其绑定外部模式
 *    2.普通集合已开启内部模式的情况下给其绑定外部模式
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建普通集合
 *    3.给集合绑定外部模式
 * 期望结果：
 *    1.集合未开启内部模式的情况下报错
 *    2.不存在冲突的情况下绑定成功，InfoSchema元数据中记录集合名，集合元数据中记录InfoSchema的名字
 *    3.ZHY TODO存在冲突的情况下报错，内部模式无任何变更，数据节点无复制日志生成
 *
 **************************************************************************************************/
main(test);

function test() {
  var disabledCLName = "disable_schema_10";
  var enabledCLName = "enable_schema_10";
  var schemaName = enabledCLName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, { column1: { Type: "string" } });

  var disabledCL = db.getCS(COMMCSNAME).createCL(disabledCLName);
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    disabledCL.addSchema(schemaName);
  });
  db.getCS(COMMCSNAME).dropCL(disabledCLName);

  var enabledCL = db.getCS(COMMCSNAME).createCL(enabledCLName, { EnableInfoSchema: true });
  enabledCL.addSchema(schemaName);
  checkIfCollectionBoundToSchema(db, COMMCSNAME, enabledCLName, schemaName);
  db.getCS(COMMCSNAME).dropCL(enabledCLName);
}
