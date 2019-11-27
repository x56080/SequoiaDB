/* *****************************************************************************
@discretion: there is a transaction operation on the cl, than rename cl
@author£º2018-10-16 wuyan  Init
***************************************************************************** */
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
function main()
{
   try
   {
      var clName = CHANGEDPREFIX + "_renameCL16070";
      var newCLName = CHANGEDPREFIX + "_newrenameCL16070";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      var dbcl = commCreateCL( db, COMMCSNAME, clName );
      
      var dataNums = 100;
      db.transBegin();
      //var dbcl = db.getCS( COMMCSNAME ).getCL( clName );
      insertData( dbcl, dataNums );
      
      var newdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      renameCLExistTrans( newdb, COMMCSNAME, clName, newCLName );
      //check the clName is oldName
      checkRenameCLResult( COMMCSNAME, newCLName, clName );
      
      db.transCommit();
      
      var dbcs = db.getCS( COMMCSNAME );
      dbcs.renameCL( clName, newCLName );
      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      var newCL = dbcs.getCL( newCLName );
      checkCount( newCL, dataNums );
      
      commDropCL( db, COMMCSNAME, newCLName, true, true, "drop CL in the ending" );
   }
   finally
   {
      if( undefined !== newdb )
      {
         newdb.close();
      }
   }
}



