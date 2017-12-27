/*******************************************************************************
*@Description : export the data by using : --sort option
*               [SEQUOIADBMAINSTREAM-502]
*@Modify list :
*               2014-11-10  xiaojun Hu  Change
*******************************************************************************/

function main( db )
{
   try
   {
      var FILENAME = LocalPath + "/" + CHANGEDPREFIX + "_export_option_sort.file",
          expCmd = InstallPath + "/bin/sdbexprt",
          impCmd = InstallPath + "/bin/sdbimprt",
          recordNum = 50,
          fieldValue = new Array(),
          NEWCLNAME = CHANGEDPREFIX + "_CL_Import_Verify",
          cmd = new Cmd() ;
      // create collection
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, true,
                             "create collection in the beginning" ) ;
      for( var i = 0 ; i < recordNum ; ++i )
      {
         fieldValue[i] = "sortfieldvalue" + i ;
         var record = new Array( { "sortString": fieldValue[i],
                                   "sortNumber": i } ) ;
         cmdInsert( cl, 1, false, record, false ) ;
      }
      println( "success to insert " + recordNum + " records" ) ;
      // create new collection
      commDropCL( db, COMMCSNAME, NEWCLNAME, true, true,
                  "failed to clean new collection" ) ;
      var newCL = commCreateCL( db, COMMCSNAME, NEWCLNAME, -1, true, true,
                                "failed to create new collection" ) ;
/*******************************************************************************
*@【Test Point1】: export data by specify sort , filt value is number
*******************************************************************************/
      var exportSort = '{ "sortNumber": -1 }' ;
      println( "sort key: " + exportSort ) ;
      var expCmdOption = "--hostname "+COORDHOSTNAME+" --svcname "+COORDSVCNAME+
                         " -c "+COMMCSNAME+" -l "+COMMCLNAME+" --file "+FILENAME+
                         " --type json --sort '" + exportSort + "'"+" --replace";
      try
      {
         cmdToolRun( expCmd, expCmdOption ) ;
      }
      catch( e )
      {
         println( "failed to export data by specify --sort, rc = " + e ) ;
         throw e ;
      }
      // verify file is sort or not
      var line = cmd.run( "wc -l " + FILENAME + " | cut -d ' ' -f 1").split('\n') ;
      var lineNum = parseInt( line[0] ) ;
      println( "record number : " + lineNum ) ;
      for( var i = 1 ; i <= lineNum ; ++i )
      {
         var oneRecord = cmd.run( "sed -n '" + i + "p' " + FILENAME ).split( "\n" ) ;
         //println( "record : " + oneRecord[0] ) ;
         //var recordObj = eval( "(" + oneRecord + ")" ) ;
         var recordObj = JSON.parse( oneRecord[0] ) ;
         if( (lineNum-i) != recordObj.sortNumber )
         {
            println( "expect number : " + (lineNum-i-1) +
                     ", actual record: " + oneRecord ) ;
            throw "ErrSortExport" ;
         }
      }
      println( "success to export record by using : --sort[number]" ) ;

      // import to new collection
      newCL.remove() ;
      var impCmdOption = "--hostname "+COORDHOSTNAME+" --svcname "+COORDSVCNAME+
                         " -c "+COMMCSNAME+" -l "+NEWCLNAME+" --file "+FILENAME+
                         " --type json" ;
      try
      {
         cmdToolRun( impCmd, impCmdOption ) ;
      }
      catch( e )
      {
         println( "failed to import data, rc = " + e ) ;
         throw e ;
      }
      // verify data. totol records is 50
      if( 50 != newCL.count() )
      {
         println( "sort: " + exportSort ) ;
         println( "all record: " + cl.count() +
                  ", export record: " + newCL.count() ) ;
         throw "ErrorVerifyOptionSortRecordString" ;
      }
      println( "success to test export data[number] by specify option --sort" ) ;
/*******************************************************************************
*@【Test Point2】: export data by specify sort , filt value is string
*******************************************************************************/
      // export data
      var exportSort = '{ "sortString": 1 }' ;
      println( "sort key: " + exportSort ) ;
      var expCmdOption = "--hostname "+COORDHOSTNAME+" --svcname "+COORDSVCNAME+
                         " -c "+COMMCSNAME+" -l "+COMMCLNAME+" --file "+FILENAME+
                         " --type json --sort '" + exportSort + "'"+" --replace";
      try
      {
         cmdToolRun( expCmd, expCmdOption ) ;
      }
      catch( e )
      {
         println( "failed to export data by specify --sort, rc = " + e ) ;
         throw e ;
      }
      // verify file is sort or not
      var line = cmd.run( "wc -l " + FILENAME + " | cut -d ' ' -f 1").split('\n') ;
      var lineNum = parseInt( line[0] ) ;
      println( "line number : " + lineNum ) ;
      var temp = "sortfieldvalue" ;
      for( var i = 1 ; i <= lineNum ; ++i )
      {
         var oneRecord = cmd.run( "sed -n '" + i + "p' " + FILENAME ).split( "\n" ) ;
         //println( "record : " + oneRecord[0] ) ;
         //var recordObj = eval( "(" + oneRecord + ")" ) ;
         var recordObj = JSON.parse( oneRecord[0] ) ;
         if( temp >= recordObj.sortString )
         {
            println( "expect number : " + (lineNum-i-1) +
                     ", actual record: " + oneRecord ) ;
            throw "ErrSortExport" ;
         }
         temp = recordObj.sortString ;
      }
      println( "success to export record by using : --sort[string]" ) ;

      // import to new collection
      newCL.remove() ;
      var impCmdOption = "--hostname "+COORDHOSTNAME+" --svcname "+COORDSVCNAME+
                         " -c "+COMMCSNAME+" -l "+NEWCLNAME+" --file "+FILENAME+
                         " --type json" ;
      try
      {
         cmdToolRun( impCmd, impCmdOption ) ;
      }
      catch( e )
      {
         println( "failed to import data, rc = " + e ) ;
         throw e ;
      }
      // verify data. totol records is 50
      if( 50 != newCL.count() )
      {
         println( "sort: " + exportSort ) ;
         println( "all record: " + cl.count() +
                  ", export record: " + newCL.count() ) ;
         throw "ErrorVerifyOptionSortRecordString" ;
      }
      println( "success to test export data[string] by specify option --sort" ) ;
      commDropCL( db, COMMCSNAME, NEWCLNAME, false, false,
                  "drop collection in the beginning" ) ;
      cmd.run( "rm -rf " + FILENAME ) ;
   }
   catch( e )
   {
      cmd.run( "rm -rf " + FILENAME ) ;
      commDropCL( db, COMMCSNAME, NEWCLNAME, true, true,
                  "drop collection in the beginning" ) ;
      throw e ;
   }

}

// Run Main
try
{
   initGlobalVar() ;
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop collection in the beginning" ) ;
   main( db ) ;
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "drop collection in the beginning" ) ;
}
catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop collection in the beginning" ) ;
   throw e ;
}
