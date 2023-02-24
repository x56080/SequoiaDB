import("../lib/basic_operation/commlib.js");
import("../lib/main.js");

testConf.skipStandAlone = true;

var DEFAULT_SCHEMA_FIELDS_DEFINE = {
  field1: { Type: "string" },
  field2: { Type: "double", ReadDefault: 10.2, WriteDefault: 5.1 },
};

var installPath = getInstallDir();

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

function insertData(dbcl) {
  function codeFunc() {
    println("code");
  }

  for (i = 0; i < 100; ++i) {
    var record = {
      minkey: MinKey(),
      double: i + 0.5,
      str: "str",
      obj: { objfield: i },
      arr: [i],
      bindata: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      undefined: undefined,
      oid: { $oid: "123abcd00ef12358902300ef" },
      date: Date(),
      null: null,
      regex: { $regex: "^张", $options: "i" },
      code: codeFunc,
      symbol: "symbol",
      int: i,
      timestamp: Timestamp(),
      long: NumberLong(i),
      decimal: { $decimal: "123" },
      maxkey: MaxKey(),
    };
    dbcl.insert(record);
  }
}

function checkIfInfoSchemaEnabled(db, csname, clName) {
  var cata = db.snapshot(SDB_SNAP_CATALOG, { Name: csname + "." + clName });
  assert.equal(JSON.parse(cata[0]).Attribute & 32, 32);
}

function checkColumnDef(db, schemaName, columnName, expectedColumnDef) {
  var r = db.list(SDB_LIST_SCHEMAS, { Name: schemaName });
  var actResult = JSON.parse(r[0]).Columns[columnName];
  var keys = Object.keys(expectedColumnDef);
  for (var i in keys) {
    var key = keys[i];
    if (!actResult.hasOwnProperty(key) || actResult[key] != expectedColumnDef[key]) {
      throw new Error(
        "\nExpected:\n" +
          JSON.stringify(expectedColumnDef) +
          "\nactual:\n" +
          JSON.stringify(actResult)
      );
    }
  }
}

function checkIfColumnNotExist(db, schemaName, columnName) {
  var r = db.list(SDB_LIST_SCHEMAS, { Name: schemaName });
  if (typeof JSON.parse(r[0])[columnName] == "object") {
    throw new Error("the column " + columnName + " still exists");
  }
}

function checkIfCollectionBoundToSchema(db, csName, clName, schemaName) {
  var clFullName = csName + "." + clName;
  var r = db.list(SDB_LIST_SCHEMAS, { Name: schemaName })[0];
  assert.equal(JSON.parse(r).Collection, clFullName);

  var r = db.snapshot(SDB_SNAP_CATALOG, { Name: clFullName })[0];
  assert.equal(JSON.parse(r).Schema, schemaName);
}

function commClearLegacySchema(db, schemaName) {
  try {
    db.dropSchema(schemaName);
  } catch (e) {
    var err = e.message || e;
    if (err != SDB_SCHEMA_NOT_EXIST) {
      throw e;
    }
  }
}

function SchemaNoColumnError(message, columnName){
  this.message = message;
  this.columnName = columnName;
}

SchemaNoColumnError.prototype = new Error();
SchemaNoColumnError.prototype.constructor = SchemaNoColumnError;

function commCheckInternalSchema(db, csName, clName, columnsDef) {
  var cursor = db.getCS(csName).getCL(clName).getInternalSchema();
  while (cursor.next()) {
    var obj = cursor.current().toObj();
    var actColumns = obj.InternalSchemas[0].Columns;
    var keys = Object.keys(columnsDef);
    for (var i in keys) {
      var columnName = keys[i];
      if (!actColumns.hasOwnProperty(columnName)) {
        throw new SchemaNoColumnError("The internal schema has no column " + columnName, columnName);
      } else {
        if (
          columnsDef[columnName].hasOwnProperty("ReadDefault") &&
          actColumns[columnName].ReadDefault != columnsDef[columnName].ReadDefault
        ) {
          throw new Error(
            "\nField " +
              columnName +
              " Expected ReadDefault:\n" +
              JSON.stringify(columnsDef[columnName].ReadDefault) +
              "\nField " +
              columnName +
              " Actual ReadDefault:\n" +
              JSON.stringify(actColumns[columnName].ReadDefault)
          );
        }
        if (
          columnsDef[columnName].hasOwnProperty("WriteDefault") &&
          actColumns[columnName].WriteDefault != columnsDef[columnName].WriteDefault
        ) {
          throw new Error(
            "\nField " +
              columnName +
              " Expected WriteDefault:\n" +
              JSON.stringify(columnsDef[columnName].WriteDefault) +
              "\nField " +
              columnName +
              " Actual WriteDefault:\n" +
              JSON.stringify(actColumns[columnName].WriteDefault)
          );
        }
      }
    }
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
  actContent.split('\n').forEach(function(text){
    var obj;
    try {
      obj = JSON.parse(text);
    } catch (error) {
      
    }
    if ( obj != null)
    {
      actObjs.push(obj);
    }
  });
  assert.equal(actObjs, expRecs);
}

function commCheckInternalSchemaHasNoColumn(db, csName, clName, columnsDef, columnNotExist)
{
  try {
    commCheckInternalSchema(db, csName, clName, columnsDef);
    throw Error("Expect to have no column " + columnNotExist + ", but it exists");
  } catch (e) {
    if (!(e instanceof SchemaNoColumnError && e.columnName == columnNotExist)) {
      throw e;
    }
  }
}