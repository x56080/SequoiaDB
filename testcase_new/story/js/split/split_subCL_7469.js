/******************************************************************************
*@Description : when collection is main CL, cann't do split
*               [BUG]SEQUOIADBMAINSTREAM-520
*@Modify list :
*               2015-2-10   xiaojun Hu Init
******************************************************************************/

function main ( db )
{
   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      println( "don't have enough group: " + groups.length +
         ", need 2 groups at least" );
      return;
   }
   var srcRG = groups[0][0]["GroupName"];
   var dstRG = groups[1][0]["GroupName"];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {
      ShardingKey: { "No": -1 },
      ShardingType: "range", Partition: 1024,
      ReplSize: 0, IsMainCL: true
   },
      true, false, "create collection in split" );
   println( "create main collection successful" );
   // split
   try
   {
      cl.split( srcRG, dstRG, 50 )
      throw "<should not run split success>";
   }
   catch( e )
   {
      if( -246 != e )
      {
         println( "failed to run split , rc = " + e );
         throw e;
      }
      else
      {
         println( "split exception: " + e );
         var errMsg = getLastErrMsg();
         if( getErr( -246 ) != errMsg )
         {
            println( "get last error message: " );
            throw errMsg;
         }
         else
         {
            println( "success to test split() when collection is mainCL" );
         }
      }
   }
   // split async
   try
   {
      cl.splitAsync( srcRG, dstRG, 50 )
      throw "<should not run split success>";
   }
   catch( e )
   {
      if( -246 != e )
      {
         println( "failed to run split , rc = " + e );
         throw e;
      }
      else
      {
         println( "splitAsync exception: " + e );
         var errMsg = getLastErrMsg();
         if( getErr( -246 ) != errMsg )
         {
            println( "get last error message: " );
            throw errMsg;
         }
         else
         {
            println( "success to test splitAsync() when collection is mainCL" );
         }
      }
   }

}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clear collection in the beginning" );
   if( false == commIsStandalone( db ) )
      main( db );
   else
      println( "WARNING, run mode is standalone" );
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clear collection in the beginning" );
   db.close();
}
catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clear collection in the beginning" );
   db.close();
   throw e;
}
