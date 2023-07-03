/***************************************************************************************************
 * @Description: 测试使用 mongoshake MongoDB 与 SequoiaDB 之间进行数据备份与恢复
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: lcx
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 *
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 *    部署 MongoDB 数据库，mongo replSet 保护端口为 27017 的节点
 *    部署 SequoiaDB 数据库，协调节点 fap 端口为 50007
 *    在每次运行测试用例之前，都需要重新搭建源端的数据库，确保源端数据库中没有 oplog
 * 测试场景：
 *    1. 增量同步
 *      2.1 启动 mongoshake 工具的增量同步模式，增量同步 MongoDB 数据库中的数据到 SequoiaDB 数据库中
 *      2.2 在同步过程中，继续向 MongoDB 数据库中插入数据
 *      2.3 所有数据插入完成后，检查 SequoiaDB 数据库中的数据是否与 MongoDB 数据库中的数据一致
 *    2. 全量同步
 *       1.1 先向 MongoDB 数据库中插入数据
 *       1.2 数据插入完成后，启动 mongoshake 工具的全量同步模式，全量同步 MongoDB 数据库中的数据到 SequoiaDB 数据库中
 *       1.3 数据同步完成后，检查 SequoiaDB 数据库中的数据是否与 MongoDB 数据库中的数据一致
 *   3. 全量同步 + 增量同步
 *     3.1 先向 MongoDB 数据库中插入数据
 *     3.2 数据插入完成后，启动 mongoshake 工具的全量同步模式，全量同步 MongoDB 数据库中的数据到 SequoiaDB 数据库中
 *     3.3 在同步过程中，继续向 MongoDB 数据库中插入数据
 *     3.4 所有数据插入完成后，检查 SequoiaDB 数据库中的数据是否与 MongoDB 数据库中的数据一致
 * 测试步骤：
 * 期望结果：
 *    正常执行不报错。
 *
 **************************************************************************************************/
import("commlib.js");

function fullTest() {
  print("\n------------------Begin fullTest-------------------\n");
  // 0. clear environment
  printCostTime(dropDatabase, [Test_DB]);
  printCostTime(stopMongoShake, []);
  printCostTime(dropMongoDB, [MongoShake_DB]);

  // 1. generate data to MongoDB
  printCostTime(genDataToMongoDB, [4]);

  // 2. start mongoshake in full mode
  printCostTime(startMongoShake, ["full", true]);

  // 3. check data in SequoiaDB
  printCostTime(checkSequoiadbData, [Test_DB, 60]);

  // 4. clear environment
  printCostTime(dropDatabase, [Test_DB]);
  printCostTime(stopMongoShake, []);

  print("\n------------------End fullTest-------------------\n");
}

function incrTest() {
  print("\n------------------Begin incrTest-------------------\n");
  // 0. clear environment
  printCostTime(dropDatabase, [Test_DB]);
  printCostTime(stopMongoShake, []);
  printCostTime(dropMongoDB, [MongoShake_DB]);

  // 1. start mongoshake in incr mode
  printCostTime(startMongoShake, ["incr", true]);

  // 2. generate data to MongoDB
  printCostTime(genDataToMongoDB, [4]);

  // 3. check data in SequoiaDB
  printCostTime(checkSequoiadbData, [Test_DB, 120]);

  // 4. clear environment
  printCostTime(dropDatabase, [Test_DB]);
  printCostTime(stopMongoShake, []);
  printCostTime(dropMongoDB, [MongoShake_DB]);
  print("\n------------------End incrTest-------------------\n");
}

function allTest() {
  print("\n------------------Begin allTest-------------------\n");
  // 0. clear environment
  printCostTime(dropDatabase, [Test_DB]);
  printCostTime(stopMongoShake, []);
  printCostTime(dropMongoDB, [MongoShake_DB]);

  // 1. generate data to MongoDB
  printCostTime(genDataToMongoDB, [4]);

  // 2. start mongoshake in full mode
  printCostTime(startMongoShake, ["all", true]);

  // 3. check data in SequoiaDB
  printCostTime(checkSequoiadbData, [Test_DB, 60]);

  sleep(2000);
  // 4. drop database in MongoDB
  printCostTime(dropMongoDB, [Test_DB]);

  // 5. wait for drop database in SequoiaDB
  printCostTime(waitSequoiaDBDrop, [Test_DB, 60]);

  // 6. generate data to MongoDB
  printCostTime(genDataToMongoDB, [4]);

  // 7. check data in SequoiaDB
  printCostTime(checkSequoiadbData, [Test_DB, 60]);

  // 8. clear environment
  printCostTime(dropDatabase, [Test_DB]);
  printCostTime(stopMongoShake, []);
  printCostTime(dropMongoDB, [MongoShake_DB]);
  print("\n------------------End allTest-------------------\n");
}

function main() {
  // The incrTest must be executed in a newly build MongoDB environment
  incrTest();
  fullTest();
  allTest();
}

main();
