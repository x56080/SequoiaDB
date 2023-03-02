/***************************************************************************************************
 * @Description: 外部模式管理：重命名主子表外部模式的字段
 * @ATCaseID: schema_27
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/23/2023 Zhou Hongye Rename schema column of main-collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    对主表的外部模式重命名，其中需构造一个子表有记录，一个子表无记录，校验其子表的内部模式字段，
 * 重命名外部模式字段，再校验子表的内部模式及记录数据
 *
 * 测试步骤：
 *    1.创建范围分区集合，且不开启内部模式
 *    2.插入含有a,b字段的记录，再开启内部模式
 *    3.直连到数据节点上，检查集合的内部模式是否符合预期
 *    4.插入含有c字段的记录，检查集合的内部模式是否符合预期
 *    5.重命名a,b字段，检查集合的内部模式是否符合预期
 *
 * 期望结果：
 *    所有校验均符合预期
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_27";
  var cl = commCreateCL(db, COMMCSNAME, clName, {
    IsMainCL: true,
    ShardingKey: { id: 1 },
    ShardingType: "range",
    EnableInfoSchema: false,
  });
  var subclName1 = clName + "_sub1";
  var subclName2 = clName + "_sub2";
  var subcl1 = commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: false });
  var subcl2 = commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: false });

  cl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { id: 0 },
    UpBound: { id: 10 },
  });
  cl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { id: 10 },
    UpBound: { id: 20 },
  });

  cl.insert({ id: 0, a: 1, b: 2 });
  cl.alter({ EnableInfoSchema: true });
  subcl1.alter({ EnableInfoSchema: true });
  subcl2.alter({ EnableInfoSchema: true });
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    a: { Type: "int32" },
    b: { Type: "int32", WriteDefault: 5, ReadDefault: 10 },
  });
  cl.addSchema(schemaName);

  // 两个子表的集合内部模式都无a字段
  checkInternalSchemaHasNoColumn(
    db,
    COMMCSNAME,
    subclName1,
    {
      b: { WriteDefault: 5, ReadDefault: 10 },
      a: {},
    },
    "a"
  );
  checkInternalSchemaHasNoColumn(
    db,
    COMMCSNAME,
    subclName2,
    {
      b: { WriteDefault: 5, ReadDefault: 10 },
      a: {},
    },
    "a"
  );

  // 插入带有字段c的记录
  cl.insert({ id: 11, a: 1, b: 2, c: 3 });

  // 子表1内部模式无a, c字段
  checkInternalSchemaHasNoColumn(db, COMMCSNAME, subclName1, { a: {} }, "a");
  checkInternalSchemaHasNoColumn(db, COMMCSNAME, subclName1, { c: {} }, "c");

  // 子表2内部模式含a,c字段
  checkInternalSchema(db, COMMCSNAME, subclName2, {
    b: { WriteDefault: 5, ReadDefault: 10 },
    a: {},
    c: {},
  });

  // 重命名a, b字段;
  schema.renameColumn("a", "new_a");
  schema.renameColumn("b", "new_b");
  // 子表1内部模式无c字段,含new_a,new_b字段
  checkInternalSchemaHasNoColumn(db, COMMCSNAME, subclName1, { c: {} }, "c");
  checkInternalSchema(db, COMMCSNAME, subclName1, {
    new_a: {},
    new_b: { WriteDefault: 5, ReadDefault: 10 },
  });

  // 子表2内部模式含new_a,new_b,c字段
  checkInternalSchema(db, COMMCSNAME, subclName2, {
    new_b: { WriteDefault: 5, ReadDefault: 10 },
    new_a: {},
    c: {},
  });

  var expRecs = [
    { id: 0, new_a: 1, new_b: 2 },
    { id: 11, new_a: 1, new_b: 2, c: 3 },
  ];
  var cursor = cl.find().sort({id:1});
  commCompareResults(cursor, expRecs);
  commDropCL(db, COMMCSNAME, clName);
}
