/***************************************************************************************************
 * @Description: 外部模式使用：给子表绑定外部模式
 * @ATCaseID: schema_13
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Bind external schema to a sub-collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.创建主子表，并给开启了内部模式的子表绑定外部模式
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建主子表
 *    3.给子表绑定外部模式
 * 期望结果：
 *    子表不可直接绑定外部模式，报错
 *
 **************************************************************************************************/
main(test);

function test() {
  var mainCLName = "maincl_schema_13";
  var maincl = commCreateCL(db, COMMCSNAME, mainCLName, {
    IsMainCL: true,
    ShardingKey: { create_date: 1 },
    ShardingType: "range",
  });
  var subclName1 = mainCLName + "_sub1";
  var subclName2 = mainCLName + "_sub2";
  var subcl1 = commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: true });
  var subcl2 = commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: true });

  maincl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { create_date: "201801" },
    UpBound: { create_date: "201901" },
  });
  maincl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { create_date: "201901" },
    UpBound: { create_date: "202001" },
  });

  var schemaName = "schema_13_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, DEFAULT_SCHEMA_FIELDS_DEFINE);
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    subcl1.addSchema(schemaName);
  });

  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    subcl2.addSchema(schemaName);
  });

  db.dropSchema(schemaName);
  commDropCL(db, COMMCSNAME, mainCLName);
}
