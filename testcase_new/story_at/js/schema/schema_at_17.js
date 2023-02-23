/***************************************************************************************************
 * @Description: 索引操作：删除索引
 * @ATCaseID: schema_17
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Drop index with schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.索引创建在外部模式已有字段
 *    2.索引创建在外部模式新增字段
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建集合并绑定外部模式
 *    3.索引创建在已有字段
 *    4.插入数据
 *    5.删除索引
 * 期望结果：
 *    索引删除成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_17";
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 3.5 },
  });
  var cl = commCreateCL(db, COMMCSNAME, clName, { EnableInfoSchema: true });
  cl.addSchema(schemaName);
  cl.createIndex("index1", { a: 1 });
  cl.createIndex("index2", { b: 1 });
  var expRecs = [];
  for (var i = 0; i < 10; ++i) {
    cl.insert({ a: i });
    expRecs.push({ a: i, b: 3.5 });
    cl.insert({ b: i + 0.1 });
    expRecs.push({ b: i + 0.1, a: 5 });
  }
  var originExpRecs = expRecs.concat();

  var cursor = cl.find().hint({ "": "index1" });
  expRecs.sort(function (left, right) {
    return left.a - right.a;
  });
  commCompareResults(cursor, expRecs);

  var cursor = cl.find().hint({ "": "index2" });
  expRecs.sort(function (left, right) {
    return left.b - right.b;
  });
  commCompareResults(cursor, expRecs);

  cl.createIndex("index3", { a: 1, b: -1 });
  var cursor = cl.find().hint({ "": "index3" });
  expRecs.sort(function (left, right) {
    return -(left.b - right.b);
  });
  expRecs.sort(function (left, right) {
    return left.a - right.a;
  });
  commCompareResults(cursor, expRecs);

  cl.dropIndex("index1");
  cl.dropIndex("index2");
  cl.dropIndex("index3");

  var cursor = cl.find();
  commCompareResults(cursor, originExpRecs);

  commDropCL(db, COMMCSNAME, clName);
}
