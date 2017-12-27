/*******************************************************************************
*@Description : export the data by using : --filter option
*               [SEQUOIADBMAINSTREAM-502]
*@Modify list :
*               2014-11-10  xiaojun Hu  Change
*******************************************************************************/

function main( db )
{
   try
   {
      var FILENAME = LocalPath + "/" + CHANGEDPREFIX + "_export_option_filter.file",
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
         fieldValue[i] = "filterfieldvalue" + i ;
         var record = new Array( { "filterString": fieldValue[i],
                                   "filterNumber": i } ) ;
         cmdInsert( cl, 1, false, record, false ) ;
      }
      println( "success to insert " + recordNum + " records" ) ;
      // create new collection
      commDropCL( db, COMMCSNAME, NEWCLNAME, true, true,
                  "failed to clean new collection" ) ;
      var newCL = commCreateCL( db, COMMCSNAME, NEWCLNAME, -1, true, true,
                                "failed to create new collection" ) ;
/*******************************************************************************
*@【Test Point1】: export data by specify filter , filter number number
*******************************************************************************/
      for( var i = 0 ; i < recordNum ; ++i )
      {
         // export data
         var exportFilter = '{ "filterNumber": { $lt: '+ i + ' }}' ;
         var expCmdOption = "--hostname "+COORDHOSTNAME+" --svcname "+COORDSVCNAME+
                            " -c "+COMMCSNAME+" -l "+COMMCLNAME+" --file "+FILENAME+
                            " --type json --filter '" + exportFilter + "'" + " --replace";
         try
         {
            cmdToolRun( expCmd, expCmdOption ) ;
         }
         catch( e )
         {
            println( "failed to export data by specify --filter, rc = " + e ) ;
            throw e ;
         }
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
         // verify data
         //if( (recordNum-i-1) != newCL.count() )
         if( i != newCL.count() )
         {
            println( "filter: " + exportFilter ) ;
            println( "all record: " + cl.count() +
                     ", export record: " + newCL.count() ) ;
            throw "ErrorVerifyOptionFilterRecordNumber" ;
         }
      }
      println( "success to test export data[number] by specify option --filter " ) ;
/*******************************************************************************
*@【Test Point2】: export data by specify filter , filt value is string
*******************************************************************************/
      // export data
      var exportFilter = '{ "filterString": { $gt: "filterfieldvalue19" }}' ;
      println( "filter key: " + exportFilter ) ;
      var expCmdOption = "--hostname "+COORDHOSTNAME+" --svcname "+COORDSVCNAME+
                         " -c "+COMMCSNAME+" -l "+COMMCLNAME+" --file "+FILENAME+
                         " --type json --filter '" + exportFilter + "'" + " --replace";
      try
      {
         cmdToolRun( expCmd, expCmdOption ) ;
      }
      catch( e )
      {
         println( "failed to export data by specify --filter, rc = " + e ) ;
         throw e ;
      }
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
      if( 38 != newCL.count() )
      {
         println( "filter: " + exportFilter ) ;
         println( "all record: " + cl.count() +
                  ", export record: " + newCL.count() ) ;
         throw "ErrorVerifyOptionFilterRecordString" ;
      }
      println( "success to test export data[string] by specify option --filter " ) ;
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
