/***************************************************************************************************
 * @Description: 内部模式管理：向已有外部模式的主表绑定子表
 * @ATCaseID: schema_28
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/27/2023 Zhou Hongye Add sub-collection to a main-collection bound to schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    向已有外部模式的主表挂载子表，新挂载的子表会同步外部模式到内部模式中
 *
 * 测试步骤：
 *    1.创建主表，预先创建并挂载两个子表，创建外部模式并挂载到主表上
 *    2.创建新的子表，挂载到主表，检查该子表的内部模式元数据是否符合预期
 *    3.插入含有新字段的记录，且该记录落在新的子表上，检查该子表的内部模式是否含有新字段
 *
 * 期望结果：
 *    新的子表的内部模式元数据符合预期
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_28";
  var cl = commCreateCL(db, COMMCSNAME, clName, {
    IsMainCL: true,
    ShardingKey: { id: 1 },
    ShardingType: "range",
    EnableInfoSchema: true,
  });
  var subclName1 = clName + "_sub1";
  var subclName2 = clName + "_sub2";
  commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: true });
  commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: true });

  cl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { id: 0 },
    UpBound: { id: 10 },
  });
  cl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { id: 10 },
    UpBound: { id: 20 },
  });

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    a: { Type: "int32" },
    b: { Type: "int32", WriteDefault: 5, ReadDefault: 10 },
  });
  cl.addSchema(schemaName);

  var subclName3 = clName + "_sub3";
  commCreateCL(db, COMMCSNAME, subclName3, { EnableInfoSchema: true });
  cl.attachCL(COMMCSNAME + "." + subclName3, {
    LowBound: { id: 20 },
    UpBound: { id: 30 },
  });

  checkInternalSchema(db, COMMCSNAME, subclName3, { b: { WriteDefault: 5, ReadDefault: 10 } });
  checkInternalSchemaHasNoColumn(db, COMMCSNAME, subclName3, { a: {} }, "a");
  cl.insert({ id: 25, c: 2.5 });
  checkInternalSchema(db, COMMCSNAME, subclName3, {
    b: { WriteDefault: 5, ReadDefault: 10 },
    c: {},
  });

  commDropCL(db, COMMCSNAME, clName);
}
