/* *****************************************************************************
@discretion: there is a transaction operation on the cl, than rename cl in transaction
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
   var clName = CHANGEDPREFIX + "_renameCL16069"; 
   var newCLName = CHANGEDPREFIX + "_newrenameCL16069"; 
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" ); 
   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" ); 
   var dbcl = commCreateCL( db, COMMCSNAME, clName ); 
   
   var dataNums = 100; 
   db.transBegin(); 
   insertData( dbcl, dataNums ); 
   
   //rename cl fail in a transction, check the clName is oldName
   var dbcs = db.getCS( COMMCSNAME ); 
   renameCLInTrans( dbcs, clName, newCLName ); 
   checkRenameCLResult( COMMCSNAME, newCLName, clName ); 
   
   db.transCommit(); 
   checkCount( dbcl, dataNums ); 
   
   //rename cl success after commit the transction
   dbcs.renameCL( clName, newCLName ); 
   checkRenameCLResult( COMMCSNAME, clName, newCLName ); 
   var newCL = dbcs.getCL( newCLName ); 
   checkCount( newCL, dataNums ); 
   
   commDropCL( db, COMMCSNAME, newCLName, true, true, "drop CL in the ending" ); 
}


