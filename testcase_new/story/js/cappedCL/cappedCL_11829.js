/************************************
*@Description: 创建固定集合，并进行split操作
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11829
**************************************/

main();

function main ()
{
   println( "---begin test---" )
   //standalone can not split
   if( true === commIsStandalone( db ) )
   {
      println( "---run mode is standalone---" );
      return;
   }
   //less two groups,can not split
   var allGroupName = commGetGroups( db, "getGroups", "", true, true, true );
   if( 1 >= allGroupName.length )
   {
      println( "---least two groups---" );
      return;
   }

   //create cappedCL
   println( "---create cappedCL---" )
   var clName = COMMCAPPEDCLNAME + "_11829";
   var srcGroup = allGroupName[0][0].GroupName;
   var tarGroup = allGroupName[1][0].GroupName;
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, Group: srcGroup };
   var dbcl = commCreateCLByOption( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   println( "---cappedCL split---" )
   //hash split
   try
   {
      dbcl.split( srcGroup, tarGroup, { Partition: 10 }, { Partition: 20 } );
      throw "ERR_CAPPED_SPLIT";
   }
   catch( e )
   {
      if( e !== -169 )
      {
         throw buildException( "cappedCL split", e, "cappedCL split", "-169", e );
      }
   }
   //range split
   try
   {
      dbcl.split( srcGroup, tarGroup, { a: 10 }, { a: 10000 } );
   }
   catch( e )
   {
      if( e !== -169 )
      {
         throw buildException( "cappedCL split", e, "cappedCL split", "-169", e );
      }
   }
   //percent split
   try
   {
      dbcl.split( srcGroup, tarGroup, 50 );
   }
   catch( e )
   {
      if( e !== -169 )
      {
         throw buildException( "cappedCL split", e, "cappedCL split", "-169", e );
      }
   }

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   println( "---end the test---" );
}