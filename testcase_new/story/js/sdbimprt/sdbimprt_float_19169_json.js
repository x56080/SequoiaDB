/************************************************************************
*@Description:  seqDB-19169: 科学计数法，底数只有小数点（如 .E+309）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/
main(); 

function main()
{  
   var type = 'json';
   var tmpPrefix = "sdbimprt_19171";
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
   var recordsNum = 400;
   var str = "";
   // 400, b value e.g: ".E+0" / ".E+1"......".E+400"
   for (var i = 0; i < recordsNum; i++)
   {
      str += "{a:" + i + ",b:0.E+" + i + "}\n";
   }

   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint( expRecsNum, findType )
{   
   println("\n---Begin to ready expect data.");
   var expRecs = [];
   if ( findType === "double" ) 
   {
      for (var i = 0; i < expRecsNum; i++)
      {
         var record = {"a": i, "b": 0};
         expRecs.push(JSON.stringify( record ));
      }
   }   
   return "[" + expRecs + "]";
}