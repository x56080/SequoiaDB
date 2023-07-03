const MongoDB_Dump = "mongodb.archive";
const SequoiaDB_Dump = "sequoiadb.archive";
const Test_DB = "mgodatagen_test";
const MongoShake_DB = "mongoshake";
const DataGen_Config_List = [
  "bigData.json",
  "bsonType.json",
  "index.json",
  "misc.json",
  "shard.json",
  // "test.json",
];
const MongoDB_Host = "localhost";
const SequoiaDB_Host = "localhost";
const MongoDB_Port = 27017;
const SequoiaDB_Port = 50000;
const SequoiaDB_FAP_Port = SequoiaDB_Port + 7;
const printOutput = false;

function genDataToMongoDB(workerNum) {
  var cmd = new Cmd();
  var configFile = "";

  for (var conf in DataGen_Config_List) {
    configFile += DataGen_Config_List[conf] + " ";
    var confDir = "./config/" + DataGen_Config_List[conf];
    print("\nBegin to generate " + DataGen_Config_List[conf] + " data to MongoDB");
    // Use mgodatagen tool to generate data
    var ret = cmd.run(
      "./tools/mgodatagen",
      " -a -f " +
        confDir +
        " -n " +
        workerNum +
        " --host=" +
        MongoDB_Host +
        " --port=" +
        MongoDB_Port
    );
    if (printOutput) print("\n" + ret + "\n");
  }

  print("\nFinish generating " + configFile + "data to MongoDB\n");
}

function genDataToSequoiaDB(workerNum) {
  var cmd = new Cmd();
  var configFile = "";

  for (var conf in DataGen_Config_List) {
    configFile += DataGen_Config_List[conf] + " ";
    var confDir = "./config/" + DataGen_Config_List[conf];
    print("\nBegin to generate " + DataGen_Config_List[conf] + " data to SequoiaDB");
    // Use mgodatagen tool to generate data
    var ret = cmd.run(
      "./tools/mgodatagen",
      " -a -f " +
        confDir +
        " -n " +
        workerNum +
        " --host=" +
        SequoiaDB_Host +
        " --port=" +
        SequoiaDB_FAP_Port
    );
    if (printOutput) print("\n" + ret + "\n");
  }

  print("\nFinish generating " + configFile + "data to SequoiaDB\n");
}

function dumpDataFromMongoDB(database) {
  var cmd = new Cmd();

  print("\nBegin to dump data from MongoDB");

  // Use mongodump tool to dump data
  var ret = cmd.run(
    "./tools/mongodump",
    // "--out='./'" + " -d " + database + " --port=" + MongoDB_Port
    "--archive=" +
      MongoDB_Dump +
      " -d " +
      database +
      " --host=" +
      MongoDB_Host +
      " --port=" +
      MongoDB_Port
  );

  if (printOutput) print("\n" + ret + "\n");

  print("\nFinish dumping data from MongoDB\n");
}

function dumpDateFromSequoiaDB(database) {
  var cmd = new Cmd();

  print("\nBegin to dump data from SequoiaDB");

  // Use mongodump tool to dump data
  var ret = cmd.run(
    "./tools/mongodump",
    // "--out='./'" + " -d " + database + " --port=" + SequoiaDB_FAP_Port
    "--archive=" +
      SequoiaDB_Dump +
      " -d " +
      database +
      " --host=" +
      SequoiaDB_Host +
      " --port=" +
      SequoiaDB_FAP_Port
  );

  if (printOutput) print("\n" + ret + "\n");

  print("\nFinish dumping data from SequoiaDB\n");
}

function restoreDataToMongoDB(archiveFile, database) {
  var cmd = new Cmd();

  print("\nBegin to restore " + archiveFile + " to MongoDB");

  // Use mongorestore tool to restore data
  var ret = cmd.run(
    "./tools/mongorestore",
    "--archive=" +
      archiveFile +
      " -d " +
      database +
      " --host=" +
      MongoDB_Host +
      " --port=" +
      MongoDB_Port
  );

  if (printOutput) print("\n" + ret + "\n");

  print("\nFinish restoring " + archiveFile + " to MongoDB\n");
}

function restoreDataToSequoiaDB(archiveFile, database) {
  var cmd = new Cmd();

  print("\nBegin to restore " + archiveFile + " to SequoiaDB");

  // Use mongorestore tool to restore data
  var ret = cmd.run(
    "./tools/mongorestore",
    "--archive=" +
      archiveFile +
      " -d " +
      database +
      " --host=" +
      SequoiaDB_Host +
      " --port=" +
      SequoiaDB_FAP_Port
  );

  if (printOutput) print("\n" + ret + "\n");

  print("\nFinish restoring " + archiveFile + " to SequoiaDB\n");
}

function waitSequoiaDBDrop(database, seconds) {
  while (seconds-- > 0) {
    sleep(1000);
    var db = new Sdb(SequoiaDB_Host, SequoiaDB_Port);
    try {
      db.getCS(database);
    } catch (e) {
      if (-34 == e) return;
    }
  }

  print("\nWait SequoiaDB drop database timeout\n");
  throw new Error("Wait SequoiaDB drop database timeout");
}

function checkSequoiadbData(database, seconds) {
  // store clConf
  var clConf = [];
  for (var ele in DataGen_Config_List) {
    var config = DataGen_Config_List[ele];
    var file = new File("./config/" + config);
    var data = file.read(file.getSize("./config/" + config));
    var tmpList = JSON.parse(data);
    for (var cl in tmpList) {
      clConf.push(tmpList[cl]);
    }
  }

  while (seconds-- > 0) {
    sleep(1000);
    checkSuccess = true;
    var clList = [];
    var cs;

    // get clList in SequoiaDB
    var db = new Sdb(SequoiaDB_Host, SequoiaDB_Port);
    try {
      cs = db.getCS(database);
    } catch (e) {
      if (1 == seconds) {
        print("Check data failed, database " + database + " not exist");
        throw new Error("Check data failed");
      }
      continue;
    }
    var clArray = cs.listCollections().toArray();
    for (var clObj in clArray) {
      clList.push(JSON.parse(clArray[clObj]).Name.split(".")[1]);
    }

    // compare clConf and clList
    for (var ele in clConf) {
      var hasCL = false;
      for (var cl in clList) {
        if (clConf[ele].collection == clList[cl]) {
          hasCL = true;
          if (clConf[ele].count != cs.getCL(clList[cl]).count()) {
            checkSuccess = false;
            if (1 == seconds) {
              print(
                "Check data failed, count is not equal, collection name is " +
                  clConf[ele].collection
              );
              throw new Error("Check data failed");
            }
            break;
          }
        }
      }
      if (!hasCL) {
        checkSuccess = false;
        if (1 == seconds) {
          print("Check data failed, missing collection name is " + clConf[ele].collection);
          throw new Error("Check data failed");
        }
        break;
      }
    }
    if (checkSuccess) {
      break;
    }
  }

  print("\nFinish checking data between confData and SequoiaDB\n");
}

function dropMongoDB(database) {
  var cmd = new Cmd();
  // Use mongo tool to drop database
  var ret = cmd.run(
    "./tools/mongo",
    " --host=" +
      MongoDB_Host +
      " --port=" +
      MongoDB_Port +
      " --eval 'db.dropDatabase()' " +
      database
  );
  if (printOutput) print("\n" + ret + "\n");
  
  print("\nDrop database " + database + " in MongoDB successfully\n");
}

function dropSequoiaDB(database) {
  // Use sdb to drop database
  var db = new Sdb(SequoiaDB_Host, SequoiaDB_Port);
  try {
    db.dropCS(database);
  } catch (error) {}
  print("\nDrop database " + database + " in SequoiaDB successfully\n");
}

function dropDatabase(database) {
  dropMongoDB(database);
  dropSequoiaDB(database);
}

function dropDumpFile() {
  var cmd = new Cmd();
  var ret = cmd.run("rm " + MongoDB_Dump);
  ret = cmd.run("rm " + SequoiaDB_Dump);

  print("\nDrop dump file successfully\n");
}

function printCostTime(func, args) {
  var startTime = new Date();
  func.apply(this, args);
  var endTime = new Date();
  var costTime = (endTime.getTime() - startTime.getTime()) / 1000;
  print(func.name + " cost time: " + costTime + "s\n");
}

function startMongoShake(mode, enableDDL) {
  var cmd = new Cmd();
  cmd.run(
    "sed -i 's/sync_mode = \\(full\\|all\\|incr\\)\\?/sync_mode = " +
      mode +
      "/g' ./config/collector.conf"
  );

  if (enableDDL)
    cmd.run(
      "sed -i 's/filter.ddl_enable = \\(true\\|false\\)\\?/filter.ddl_enable = true/g' ./config/collector.conf"
    );
  else
    cmd.run(
      "sed -i 's/filter.ddl_enable = \\(true\\|false\\)\\?/filter.ddl_enable = false/g' ./config/collector.conf"
    );

  var ret = cmd.start("./tools/collector.linux -conf=./config/collector.conf");

  if (printOutput) print("\n" + ret + "\n");

  print("\nStart mongoshake successfully\n");
}

function stopMongoShake() {
  var cmd = new Cmd();
  try {
    var ret = cmd.run("nohup pkill -9 collector");
    cmd.run("rm -r ./diagnostic");
    cmd.run("rm ./mongoshake.pid");
  } catch (error) {}

  if (printOutput) print("\n" + ret + "\n");

  print("\nStop mongoshake successfully\n");
}
