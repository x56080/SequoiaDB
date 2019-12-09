/* *****************************************************************************
@discretion: there is a transaction operation on the cl1, than rename cl2, the cl1 and cl2 on the same cs
@author��2018-10-16 wuyan  Init
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
function main ()
{

   var csName = CHANGEDPREFIX + "_renamecs16071";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var clName1 = CHANGEDPREFIX + "_renameCL16071a";
   var newCLName1 = CHANGEDPREFIX + "_newrenameCL16071a";
   var clName2 = CHANGEDPREFIX + "_renameCL16071b";
   var dbcs = commCreateCS( db, csName, false, "Failed to create CS." );
   commCreateCL( db, csName, clName1 );
   var dbcl = commCreateCL( db, csName, clName2 );

   var dataNums = 100;
   db.transBegin();
   insertData( dbcl, dataNums );

   dbcs.renameCL( clName1, newCLName1 );

   db.transCommit();

   checkRenameCLResult( csName, clName1, newCLName1 );

   checkCount( dbcl, dataNums );

   commDropCS( db, csName, true, "drop CS in the ending" );
}

