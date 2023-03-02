/***************************************************************************************************
 * @Description: 外部模式管理：重命名数据库分区集合外部模式的字段
 * @ATCaseID: schema_26
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/23/2023 Zhou Hongye Rename schema column of sharding collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    对数据库分区集合的外部模式重命名，其中需构造一个分区有记录，一个分区无记录再开启内部模式，
 * 直连节点后校验其内部模式字段，重命名外部模式字段，再校验内部模式及记录数据
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
testConf.clName = "schema_26";
testConf.clOpt = { ShardingKey: { id: 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
main(test);

function test(testPara) {
  var clName = testConf.clName;
  var cl = testPara.testCL;
  var srcGroup = testPara.srcGroupName;
  var dstGroups = testPara.dstGroupNames;
  assert.equal(dstGroups.length > 0, true);

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    a: { Type: "int32" },
    b: { Type: "int32", WriteDefault: 5, ReadDefault: 10 },
  });

  cl.insert({ id: 0, a: 1, b: 2 });
  cl.split(srcGroup, dstGroups[0], { id: 10 });
  cl.alter({ EnableInfoSchema: true });
  cl.addSchema(schemaName);

  // 直连到数据节点，检查内部模式
  var data1 = db.getRG(srcGroup).getMaster().connect();
  var data2 = db.getRG(dstGroups[0]).getMaster().connect();
  // 两个复制组的集合内部模式都无a字段
  checkInternalSchemaHasNoColumn(
    data1,
    COMMCSNAME,
    clName,
    {
      b: { WriteDefault: 5, ReadDefault: 10 },
      a: {},
    },
    "a"
  );
  checkInternalSchemaHasNoColumn(
    data2,
    COMMCSNAME,
    clName,
    {
      b: { WriteDefault: 5, ReadDefault: 10 },
      a: {},
    },
    "a"
  );

  // 插入带有字段c的记录
  cl.insert({ id: 11, a: 1, b: 2, c: 3 });

  // 源复制组的集合内部模式无a, c字段
  checkInternalSchemaHasNoColumn(data1, COMMCSNAME, clName, { a: {} }, "a");
  checkInternalSchemaHasNoColumn(data1, COMMCSNAME, clName, { c: {} }, "c");

  // 目的复制组的集合内部模式含a,c字段
  checkInternalSchema(data2, COMMCSNAME, clName, {
    b: { WriteDefault: 5, ReadDefault: 10 },
    a: {},
    c: {},
  });

  // 重命名a, b字段;
  schema.renameColumn("a", "new_a");
  schema.renameColumn("b", "new_b");
  // 源复制组的集合内部模式无c字段,含new_a,new_b字段
  checkInternalSchemaHasNoColumn(data1, COMMCSNAME, clName, { c: {} }, "c");
  checkInternalSchema(data1, COMMCSNAME, clName, {
    new_a: {},
    new_b: { WriteDefault: 5, ReadDefault: 10 },
  });

  // 目的复制组的集合内部模式含new_a,new_b,c字段
  checkInternalSchema(data2, COMMCSNAME, clName, {
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
