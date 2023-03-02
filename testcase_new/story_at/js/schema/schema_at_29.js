/***************************************************************************************************
 * @Description: 内部模式管理：向已有外部模式的主表绑定子表，且该子表已有索引与外部模式冲突
 * @ATCaseID: schema_29
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/27/2023 Zhou Hongye Add sub-collection to a main-collection bound to schema and check index
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    向已有外部模式的主表挂载子表，且主表预先创建了索引
 *
 * 测试步骤：
 *    1.创建主表，预先创建并挂载两个子表，创建外部模式并绑定到主表上
 *    2.创建新的子表，为其创建索引，挂载到该主表
 *
 * 期望结果：
 *    1.如果子表索引字段在外部模式中指定了读默认值，预期报错
 *    2.如果子表索引字段在外部模式中存在但未指定读默认值，预期挂载成功
 *    2.如果子表索引字段在外部模式中未定义，预期挂载成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_29";
  var cl = commCreateCL(db, COMMCSNAME, clName, {
    IsMainCL: true,
    ShardingKey: { id: 1 },
    ShardingType: "range",
    EnableInfoSchema: true,
  });
  var subclName1 = clName + "_sub1";
  commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: true });

  cl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { id: 0 },
    UpBound: { id: 10 },
  });

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    a: { Type: "int32", WriteDefault: 5, ReadDefault: 10 },
    b: { Type: "int32", WriteDefault: 5 },
    c: { Type: "int32" },
  });
  cl.addSchema(schemaName);

  var subclName2 = clName + "_sub2";
  var subcl2 = commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: true });
  subcl2.createIndex("index2", { a: 1 });
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function(){
    cl.attachCL(COMMCSNAME + "." + subclName2, {
      LowBound: { id: 10 },
      UpBound: { id: 20 },
    });
  });
 

  var subclName3 = clName + "_sub3";
  var subcl3 = commCreateCL(db, COMMCSNAME, subclName3, { EnableInfoSchema: true });
  subcl3.createIndex("index3", { b: 1 });
  cl.attachCL(COMMCSNAME + "." + subclName3, {
    LowBound: { id: 20 },
    UpBound: { id: 30 },
  });
  checkInternalSchema(db, COMMCSNAME, subclName3, {
    a: { WriteDefault: 5, ReadDefault: 10 },
    b: { WriteDefault: 5 },
  });

  var subclName4 = clName + "_sub4";
  var subcl4 = commCreateCL(db, COMMCSNAME, subclName4, { EnableInfoSchema: true });
  subcl4.createIndex("index4", { c:1 });
  cl.attachCL(COMMCSNAME + "." + subclName4, {
    LowBound: { id: 30 },
    UpBound: { id: 40 },
  });
  checkInternalSchema(db, COMMCSNAME, subclName4, {
    a: { WriteDefault: 5, ReadDefault: 10 },
    b: { WriteDefault: 5 },
    c: {},
  });

  commDropCL(db, COMMCSNAME, clName);
}