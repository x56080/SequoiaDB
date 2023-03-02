/***************************************************************************************************
 * @Description: 内部模式管理:查看主子表的内部模式
 * @ATCaseID: schema_20
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/17/2023 Zhou Hongye Check the internal schema of main-collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.查看表分区集合的内部模式
 * 测试步骤：
 *    1.创建主子表
 *    2.绑定外部模式
 *    3.插入数据
 *    4.检查内部模式
 * 期望结果：
 *    内部模式各字段的默认值符合预期
 *
 **************************************************************************************************/
main(test);

function test() {
  var mainCLName = "schema_20";
  var cl = commCreateCL(db, COMMCSNAME, mainCLName, {
    IsMainCL: true,
    ShardingKey: { create_date: 1 },
    ShardingType: "range",
    EnableInfoSchema: true,
  });
  var subclName1 = mainCLName + "_sub1";
  var subclName2 = mainCLName + "_sub2";
  commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: true });
  commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: true });

  cl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { create_date: "201801" },
    UpBound: { create_date: "201901" },
  });
  cl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { create_date: "201901" },
    UpBound: { create_date: "202001" },
  });

  var schemaName = mainCLName + "_1";
  commClearLegacySchema(db, schemaName);
  var columnsDef = {
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 10.5 },
  };
  db.createSchema(schemaName, columnsDef);

  cl.addSchema(schemaName);
  for (var i = 201801; i < 202001; ++i) {
    cl.insert({ create_date: i.toString() });
  }
  checkInternalSchema(db, COMMCSNAME, mainCLName, columnsDef);
}
