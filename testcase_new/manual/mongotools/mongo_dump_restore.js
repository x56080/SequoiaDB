/***************************************************************************************************
 * @Description: 测试使用 mongodump 和 mongorestore 工具在 MongoDB 与 SequoiaDB 之间进行数据备份与恢复
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
 * 环境准备：部署 MongoDB 数据库，mongos 的端口为 27017；部署 SequoiaDB 数据库，协调节点端口为 50000
 * 测试场景：
 *    1. 使用 mongodump 工具导出 MongoDB 数据库中的数据 mongo.archive
 *    2. 使用 mongodump 工具导出 SequoiaDB 数据库中的数据 sequoiadb.archive
 *    3. 使用 mongorestore 工具把 mongo.archive 恢复到 SequoiaDB 数据库中
 *    4. 使用 mongorestore 工具把 sequoiadb.archive 恢复到 MongoDB 数据库中
 *    5. 使用 mongorestore 工具把 sequoiadb.archive 恢复到 SequoiaDB 数据库中
 * 测试步骤：
 * 期望结果：
 *    正常执行不报错。
 *
 **************************************************************************************************/
import("commlib.js");

function test() {
  // 0. drop data
  printCostTime(dropDatabase, [Test_DB]);

  // 1. generate data to MongoDB
  printCostTime(genDataToMongoDB, [4]);

  // 2. dump data from MongoDB
  printCostTime(dumpDataFromMongoDB, [Test_DB]);

  // 3. restore data to SequoiaDB
  printCostTime(restoreDataToSequoiaDB, [MongoDB_Dump, Test_DB]);

  // 4. check data
  printCostTime(checkSequoiadbData, [Test_DB]);

  // 5. drop data
  printCostTime(dropDatabase, [Test_DB]);

  // 6. generate data to SequoiaDB
  printCostTime(genDataToSequoiaDB, [4]);

  // 7. dump data from SequoiaDB
  printCostTime(dumpDateFromSequoiaDB, [Test_DB]);

  // 8. drop data
  printCostTime(dropDatabase, [Test_DB]);

  // 9. restore data to MongoDB
  printCostTime(restoreDataToMongoDB, [SequoiaDB_Dump, Test_DB]);

  // 10. restore data to SequoiaDB
  printCostTime(restoreDataToSequoiaDB, [SequoiaDB_Dump, Test_DB]);

  // 11. drop data
  printCostTime(dropDatabase, [Test_DB]);

  // 12. drop dump file
  printCostTime(dropDumpFile, []);
}

function main() {
  test();
}

main();
