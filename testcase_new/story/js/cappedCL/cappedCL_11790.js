/************************************
*@Description:capped cl,pop record,logicalID Non_existent
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11790
**************************************/
function main ()
{
   var clName = COMMCAPPEDCLNAME + "_11790";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCLByOption( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   var recordNum = 10;
   var string = "a";
   var stringLength = 1;
   insertFixedLengthDatas( dbcl, recordNum, stringLength, string );

   var sortConf = { _id: -1 };
   var limitConf = 1;
   var logicalIDs = getLogicalID( dbcl, null, null, sortConf, limitConf, null );

   pop( dbcl, logicalIDs[0], -1 );

   popCheckLogicalID( dbcl, logicalIDs[0], -1 );
   popCheckLogicalID( dbcl, logicalIDs[0], 1 );

   popCheckLogicalID( dbcl, logicalIDs[0] + 1, -1 );
   popCheckLogicalID( dbcl, logicalIDs[0] + 1, 1 );

   //SEQUOIADBMAINSTREAM-2575,补充测试
   //_id: 0,1024,2048,3072,4096 increasing
   removeAllDatas( dbcl );
   stringLength = 968;
   insertFixedLengthDatas( dbcl, recordNum, stringLength, string );

   sortConf = { _id: 1 };
   limitConf = 2;
   logicalIDs = getLogicalID( dbcl, null, null, sortConf, limitConf, null );

   //pop from 1024 and check
   pop( dbcl, logicalIDs[1], -1 );
   popCheckLogicalID( dbcl, logicalIDs[1], -1 );

   //pop from 0 and check
   pop( dbcl, logicalIDs[0], -1 );
   popCheckLogicalID( dbcl, logicalIDs[0], -1 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

main();

function popCheckLogicalID ( dbcl, logicalID, direction )
{
   try
   {
      dbcl.pop( { LogicalID: logicalID, Direction: direction } ).toArray();
      throw "NEED_THROE_ERROR";
   } catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "pop", e, "pop", -6, e );
      }
   }
}