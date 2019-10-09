/************************************************************************
*@Description:  seqDB-19174:浮点数特殊值测试（如0. / .0 / . / 00）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/
main(); 

function main()
{  
   var type = 'json';
   var tmpPrefix = "sdbimprt_19174";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix +"." + type;
   
   // init import file and expect records
   var recsNum = initImportFile_testPoint( importFile ); 
   // import
   var rc = importData( csName, clName, importFile, type ); 
   // check results
   checkImportRC( rc, recsNum );   
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for (var i = 0; i < findTypeArr.length; i++)
   {
      println("\n---------------------import data, findType is "+ findTypeArr[i] + "---------------------");
      var expRecs = initExpectData_testPoint( recsNum, findTypeArr[i] );
      var findCond = {"b": {"$type": 2, "$et": findTypeArr[i]}};
      var expRecsNum = JSON.parse( expRecs ).length;
      checkCLData( cl, expRecsNum, expRecs, findCond );
   }
   
   // clean data
   cmd.run( "rm -rf " +  importFile );
   cleanCL( csName, clName );
}

function initImportFile_testPoint( importFile )
{
   println("\n---Begin to ready import file.");
   var file = fileInit( importFile );
   var tmpNum = 400;
   var recordsNum = tmpNum * 3 + 1;
   
   // 0, b value e.g: "0." / "00."......
   var str = "";
   var bVal = "0.";
   for (var i = 0; i < tmpNum; i++)
   {
      str += "{a:" + i + ",b:" + bVal + "}\n";
      bVal = "0" + bVal;
   }
   
   // 400, b value e.g: ".0" / ".00"......
   var bVal = ".0";
   for (var i = tmpNum; i < tmpNum * 2; i++)
   {
      str += "{a:" + i + ",b:" + bVal + "}\n";
      bVal += "0";
   }
   
   // 800, b value e.g: "0" / "00"......
   var bVal = "0";
   for (var i = tmpNum * 2; i < tmpNum * 3; i++)
   {
      str += "{a:" + i + ",b:" + bVal + "}\n";
      bVal += "0";
   }
   
   // 1201, b value e.g: "."
   str += "{a:" + (tmpNum * 3) + ",b:.}\n";

   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint( expRecsNum, findType )
{   
   println("\n---Begin to ready expect data.");
   var expRecs = [];
   if ( findType === "int32" ) 
   {
      for (var i = 0; i < 400; i++)
      {
         var record = {"a": i, "b": 0};
         expRecs.push(JSON.stringify( record ));
      }
      
      for (var i = 800; i < 1200; i++)
      {
         var record = {"a": i, "b": 0};
         expRecs.push(JSON.stringify( record ));
      }
   }
   else if ( findType === "double" ) 
   {
      for (var i = 400; i < 800; i++)
      {
         var record = {"a": i, "b": 0};
         expRecs.push(JSON.stringify( record ));
      }      
   }
   
   return "[" + expRecs + "]";
}