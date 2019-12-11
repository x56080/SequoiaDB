/* *****************************************************************************
@discretion: test alter clname
@author��2018-4-16 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclName14934";

try
{
   main( db );
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ( db )
{
   try
   {
      //clean environment before test
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

      //create cl
      var dbcl = commCreateCL( db, COMMCSNAME, clName );

      //alter clName
      alterCLName( dbcl );

      //clean
      commDropCL( db, COMMCSNAME, clName, true, true,
         "clear collection in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function alterCLName ( dbcl )
{
   try
   {
      var clFullName = COMMCSNAME + "." + clName;
      dbcl.setAttributes( { "Name": clFullName } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }
}
