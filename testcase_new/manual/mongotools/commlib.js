const MongoDB_Dump = "mongodb.archive";
const SequoiaDB_Dump = "sequoiadb.archive";
const Test_DB = "mgodatagen_test";
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
  }

  print("\nFinish generating " + configFile + "data to MongoDB\n");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }
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
  }

  print("\nFinish generating " + configFile + "data to SequoiaDB\n");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }
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

  print("\nFinish dumping data from MongoDB\n");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }
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

  print("\nFinish dumping data from SequoiaDB\n");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }
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

  print("\nFinish restoring " + archiveFile + " to MongoDB\n");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }
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

  print("\nFinish restoring " + archiveFile + " to SequoiaDB\n");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }
}

function checkSequoiadbData(database) {
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

  // get clList in SequoiaDB
  var db = new Sdb(SequoiaDB_Host, SequoiaDB_Port);
  var cs = db.getCS(database);
  var clList = [];
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
          print(
            "Check data failed, count is not equal, collection name is " + clConf[ele].collection
          );
          // return false;
        }
        break;
      }
    }
    if (!hasCL) {
      print("Check data failed, missing collection name is " + clConf[ele].collection);
    }
  }

  print("\nCheck data between confData and SequoiaDB successfully\n");
}

function dropDatabase(database) {
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
  print("\nDrop database " + database + " in MongoDB successfully");

  if (printOutput) {
    print("\n");
    print(ret);
    print("\n");
  }

  // Use sdb to drop database
  var db = new Sdb(SequoiaDB_Host, SequoiaDB_Port);
  try {
    db.dropCS(database);
  } catch (error) {}
  print("\nDrop database " + database + " in SequoiaDB successfully\n");
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
