/************************************
*@Description: Create CappedCL , check Parameter AutoIndexId
*@author:      liuxiaoxuan
*@createdate:  2017.8.15
*@testlinkCase:seqDB-12383
**************************************/

main()

function main ()
{
   //Not Exist Parameter AutoIndexId
   var clName = COMMCAPPEDCLNAME + "_12383";
   var option1 = { Capped: true, Size: 1024 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName, option1, true );
   db.getCS( COMMCAPPEDCSNAME ).dropCL( clName );

   // AutoIndexId true
   var option2 = { Capped: true, Size: 1024, AutoIndexId: true };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName, option2 );

   // AutoIndexId is int
   var option3 = { Capped: true, Size: 1024, AutoIndexId: 123 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName, option3 );

   // AutoIndexId is String
   var option4 = { Capped: true, Size: 1024, AutoIndexId: "a" };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName, option4 );

   // AutoIndexId is null
   var option5 = { Capped: true, Size: 1024, AutoIndexId: null };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName, option5 );

   // Valid AutoIndexId
   var isValid = true;
   var option6 = { Capped: true, Size: 1024, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName, option6, isValid );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" )
}

function checkCreateCLOptions ( csName, clName, options, isValid )
{
   try
   {
      db.getCS( csName ).createCL( clName, options );
      if( isValid == undefined ) 
      {
         throw "NEED_CREATE_FAIL_ERROR";
      }
      println( "Create CL with option: " + JSON.stringify( options ) + " success!" );
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "Invalid parameter is not -6,error msg is: " + e );
      }
      else
      {
         println( "check result success!" );
      }
   }
}