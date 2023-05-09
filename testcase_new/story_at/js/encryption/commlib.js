import("../lib/basic_operation/commlib.js");
import("../lib/main.js");

testConf.skipStandAlone = true;
var installPath = getInstallDir();
ensureInitializedSecurityKeys();

function getInstallDir() {
  var localDir = cmd.run("pwd").split("\n")[0] + "/";
  var installDir = "";

  try {
    cmd.run("find ./bin/sdbexprt").split("\n")[0];
    installDir = localDir;
  } catch (e) {
    installDir = commGetInstallPath() + "/";
  }

  return installDir;
}

function ensureInitializedSecurityKeys() {
  var cata = db.getCataRG().getMaster().connect();
  var matcher = {Type: "GLOBAL"};
  var cl = cata.getCS("SYSINFO").getCL("SYSDCBASE");
  var cursor = cl.find( matcher );
  var hasKey = cursor.current().toObj().hasOwnProperty("MKIndex");
  if ( !hasKey )
  {
    db.initSecurityKeys();
  }
}

function checkEncrypted(db, COMMCSNAME, clName) {
  var cursor = db.snapshot(SDB_SNAP_CATALOG, {
    Name: COMMCSNAME + "." + clName,
  });
  var desc = cursor.current().toObj().AttributeDesc;
  if (desc.indexOf("Encrypted") == -1) {
    throw Error(
      "the catalog info of collection " +
        COMMCSNAME +
        "." +
        clName +
        " is not encrypted"
    );
  }
}

function checkUnencrypted(db, COMMCSNAME, clName) {
  var cursor = db.snapshot(SDB_SNAP_CATALOG, {
    Name: COMMCSNAME + "." + clName,
  });
  var desc = cursor.current().toObj().AttributeDesc;
  if (desc.indexOf("Encrypted") != -1) {
    throw Error(
      "the catalog info of collection " +
        COMMCSNAME +
        "." +
        clName +
        " is not encrypted"
    );
  }
}

function checkCsvFileContent(csvFile, expRecs, fieldNames) {
  var expContent = fieldNames.join(",");
  expContent += "\n";
  for (var i in expRecs) {
    var record = expRecs[i];
    var values = [];
    Object.keys(fieldNames).forEach(function (index) {
      var ele = fieldNames[index];
      if (record.hasOwnProperty(ele)) {
        if (typeof record[ele] == "string") {
          values.push('"' + record[ele] + '"');
        } else {
          values.push(record[ele]);
        }
      } else {
        values.push("");
      }
    });
    expContent += values.join(",");
    expContent += "\n";
  }

  var size = parseInt(File.stat(csvFile).toObj().size);
  var file = new File(csvFile);
  var actContent = file.read(size);
  file.close();
  assert.equal(actContent, expContent);
}

function checkJsonFileContent(jsonFile, expRecs) {
  var size = parseInt(File.stat(jsonFile).toObj().size);
  var file = new File(jsonFile);
  var actContent = file.read(size);
  file.close();
  var actObjs = [];
  actContent.split("\n").forEach(function (text) {
    var obj;
    try {
      obj = JSON.parse(text);
    } catch (error) {}
    if (obj != null) {
      actObjs.push(obj);
    }
  });
  assert.equal(actObjs, expRecs);
}

function checkFileContent(filePath, expContent) {
  var size = parseInt(File.stat(filePath).toObj().size);
  var file = new File(filePath);
  var actContent = file.read(size);
  file.close();
  assert.equal(actContent, expContent);
}