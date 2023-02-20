/***************************************************************************************************
 * @Description: 外部模式使用：给主表绑定外部模式，并增加字段、修改字段、删除字段
 * @ATCaseID: schema_12
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Bind external schema to a main collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.全部子表都开启内部模式
 *    2.部分或全部子表未开启内部模式
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建主子表
 *    3.给主表绑定外部模式
 *    4.增加字段、修改字段、删除字段
 * 期望结果：
 *    1.全部子表都开启了内部模式，且外部模式不与任何一个内部模式冲突的情况下，绑定成功
 *    2.其他情况均报错
 *    3.外部模式的字段变更正确
 *
 **************************************************************************************************/
main(test);

function test() {
  var mainCLName = "maincl_schema_12";
  // 全部子表都开启内部模式
  var maincl = commCreateCL(db, COMMCSNAME, mainCLName, {
    IsMainCL: true,
    ShardingKey: { create_date: 1 },
    ShardingType: "range",
    EnableInfoSchema: true,
  });
  var subclName1 = mainCLName + "_sub1";
  var subclName2 = mainCLName + "_sub2";
  commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: true });
  commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: true });

  maincl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { create_date: "201801" },
    UpBound: { create_date: "201901" },
  });
  maincl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { create_date: "201901" },
    UpBound: { create_date: "202001" },
  });

  var schemaName = "schema_12_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, DEFAULT_SCHEMA_FIELDS_DEFINE);
  maincl.addSchema(schemaName);

  // 增加字段
  var columnName = "new_field";
  schema.addColumn(columnName, {
    Type: "int32",
    ReadDefault: 5,
    WriteDefault: 10,
  });
  checkColumnDef(db, schemaName, columnName, {
    Type: "int32",
    ReadDefault: 5,
    WriteDefault: 10,
  });

  // 修改字段
  schema.alterColumn(columnName, {
    WriteDefault: 20,
  });
  checkColumnDef(db, schemaName, columnName, {
    Type: "int32",
    ReadDefault: 5,
    WriteDefault: 20,
  });

  // 删除字段
  schema.dropColumn(columnName);

  commDropCL(db, COMMCSNAME, mainCLName);

  // 部分或全部子表未开启内部模式
  var maincl = db.getCS(COMMCSNAME).createCL(mainCLName, {
    IsMainCL: true,
    ShardingKey: { create_date: 1 },
    ShardingType: "range",
  });
  var subclName1 = mainCLName + "_sub1";
  var subclName2 = mainCLName + "_sub2";
  db.getCS(COMMCSNAME).createCL(subclName1);
  db.getCS(COMMCSNAME).createCL(subclName2, { EnableInfoSchema: true });

  maincl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { create_date: "201801" },
    UpBound: { create_date: "201901" },
  });
  maincl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { create_date: "201901" },
    UpBound: { create_date: "202001" },
  });

  var schemaName = "schema_12_2";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, DEFAULT_SCHEMA_FIELDS_DEFINE);
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    maincl.addSchema(schemaName);
  });

  commDropCL(db, COMMCSNAME, mainCLName);
  db.dropSchema(schemaName);
}
