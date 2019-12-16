/************************************
*@Description: pop empty cappedCL
*@author:      liuxiaoxuan
*@createdate:  2017.8.28
*@testlinkCase: seqDB-12565
**************************************/
function main ()
{
   var clName = COMMCAPPEDCLNAME + "_12565";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, true, true );

   checkPopResult( dbcl, 0, 1 );
   checkPopResult( dbcl, 0, -1 );
   checkPopResult( dbcl, 100, -1 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}
main();

function checkPopResult ( dbcl, logicalID, direction )
{
   try
   {
      dbcl.pop( { LogicalID: logicalID, Direction: direction } ).toArray();
      throw "NEED_POP_ERROR";
   } catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "pop", e, "pop", -6, e );
      }
   }
}