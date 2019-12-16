/*******************************************************************************
*@Description : matches testcase common functions and varialb
*@Modify list :
*              2016-5-16 xiaoni huang
*******************************************************************************/
function readyCL ( clName )
{
   println( "\n---Begin to create CL." );

   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "Failed to create CL." );
   return cl;
}

function cleanCL ( clName )
{
   println( "\n---Begin to drop CL." );

   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}