/*******************************************************************************
*@Description : matches testcase common functions and varialb
*@Modify list :
*              2017-02-28 xiaoni huang
*******************************************************************************/
var cmd  = cmdInit();

function readyCL( clName )
{
   println("\n---Begin to create CL.");
	
   commDropCL( db, COMMCSNAME, clName, true, true,
               "Failed to drop CL in the pre-condition." );
   
   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
                          "Failed to create CL." );                          
   return cl;
}

function cleanCL( clName )
{
   println("\n---Begin to drop CL.");
	
   commDropCL( db, COMMCSNAME, clName, false, false,
               "Failed to drop CL in the end-condition" );
}

function cmdInit()
{
   try
   {
      var cmd = new Cmd();
      return cmd;
   }
   catch( e )
   {
      println("Failed to init cmd.");
      throw e;
   }
}

function cmdRun( str )
{
   try
   {
      var rc = cmd.run( str ).split("\n")[0];
      return rc;
   }
   catch( e )
   {
      println("Failed to exec cmd run.");
      throw e;
   }
}

/* ****************************************************
@description: turn to local time
@parameter:
   time: Timestamp with time zone to millisecond,eg:'1901-12-31T15:54:03.000Z'
   format: eg:%Y-%m-%d-%H.%M.%S.000000
@return: 
   localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime( time, format )
{
   if ( typeof( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   try
   {
      var msecond = new Date( time ).getTime();  
      var second  = parseInt( msecond / 1000 );  //millisecond to second
      var localtime  = cmdRun( 'date -d@"'+ second +'" "+'+ format +'"' );
      
      return localtime;
   }
   catch( e )
   {
      println("Timestamp with time zone to local time failed.");
      throw e;
   }
}