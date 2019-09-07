/************************************************************************
*@Description:  seqDB-19175:科学计数法，底数只有整数，且全为0（如000E+308）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/
main(); 

function main()
{  
   var type = 'csv';
   var tmpPrefix = "sdbimprt_19175";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix +"." + type;
   
   // init import file and expect records
   var recsNum = initImportFile_testPoint( importFile ); 
   // import and check results, cover type: int, long, double, decimal
   var bValTypeArr = ["int", "long", "double", "decimal"];
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for (var i = 0; i < bValTypeArr.length; i++)
   {
      println("\n---------------------import data, type is "+ bValTypeArr[i]  +", test point "+ (i+1) +"---------------------");
      // import
      var importFields = 'a int, b ' + bValTypeArr[i];
      var rc = importData( csName, clName, importFile, type, importFields, true ); 
      // check results
      checkImportRC( rc, recsNum ); 
      var expRecs = '[{"a":0,"b":0},{"a":1,"b":0}]';
      if ( bValTypeArr[i] === "decimal" ) {
         expRecs = '[{"a":0,"b":{"$decimal":"0"}},{"a":1,"b":{"$decimal":"0"}}]';
      }
      var findCond = {"b": {"$type": 2, "$et": findTypeArr[i]}};
      checkCLData( cl, recsNum, expRecs, findCond );   
   }
      
   // clean data
   cmd.run( "rm -rf " +  importFile );
   cleanCL( csName, clName );
}

function initImportFile_testPoint( importFile )
{
   println("\n---Begin to ready import file.");
   var recordsNum = 2;
   var str = "0,000E+308" + "\n" + "1,000E+400";
   var file = fileInit( importFile );
   file.write( str );
   file.close();   
   return recordsNum;
}