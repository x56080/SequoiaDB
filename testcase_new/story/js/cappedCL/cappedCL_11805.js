/************************************
*@Description:capped cl findandUpdate/findandRemove
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11805
**************************************/
function main ()
{
   var clName = COMMCAPPEDCLNAME + "_11805";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   try
   {
      dbcl.find().update( { $set: { a: 1 } } ).toArray();
      throw "NEED_THROE_ERROR";
   }
   catch( e )
   {
      if( e !== -279 )
      {
         throw buildException( "find and remove", e, null, -279, e );
      }
      else
      {
         println( "check result is ok!" );
      }
   }

   try
   {
      dbcl.find().remove().toArray();
      throw "NEED_THROE_ERROR";
   }
   catch( e )
   {
      if( e !== -279 )
      {
         throw buildException( "find and remove", e, null, -279, e );
      }
      else
      {
         println( "check result is ok!" );
      }
   }

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

main();