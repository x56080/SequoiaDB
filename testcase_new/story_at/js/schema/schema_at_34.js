/***************************************************************************************************
 * @Description: 主子表的分区键的一个列定义在schema中且有读默认值，再以该列建立单字段索引和复合索引
 * @ATCaseID: schema_34
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 03/02/2023 Zhou Hongye Create indexes on main-collection bound to schema which defines column read default
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    主子表的分区键的一个列定义在schema中且有读默认值，再以该列建立索引和复合索引
 *
 * 测试步骤：
 *    1.建立主子表，分区键指定为a,b
 *    2.为主表绑定外部模式，定义了字段b的读默认值
 *    3.在主表上为字段b建立索引
 *    4.在主表上为字段b,c建立复合索引
 *
 * 期望结果：
 *    索引创建成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_34";
  var cl = commCreateCL(db, COMMCSNAME, clName, {
    IsMainCL: true,
    ShardingKey: { a: 1, b: 1 },
    ShardingType: "range",
    EnableInfoSchema: true,
  });

  var subclName1 = clName + "_sub1";
  var subcl1 = commCreateCL(db, COMMCSNAME, subclName1, {
    EnableInfoSchema: true,
  });

  var subclName2 = clName + "_sub2";
  var subcl2 = commCreateCL(db, COMMCSNAME, subclName2, {
    EnableInfoSchema: true,
  });

  cl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { a: 0, b: 0 },
    UpBound: { a: 5, b: 5 },
  });
  cl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { a: 5, b: 5 },
    UpBound: { a: 10, b: 10 },
  });

  var schemaName = clName + "_schema";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32" },
    b: { Type: "int32", ReadDefault: 2 },
    c: { Type: "double" },
  });
  cl.addSchema(schemaName);

  cl.createIndex("index1", { b: 1 });
  cl.createIndex("index2", { b: 1, c: 1 });
}
