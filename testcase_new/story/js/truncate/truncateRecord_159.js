/******************************************************************************
*@Description: 测试水平分区表写入大于多个数据页的普通记录，然后再truncate*                        
*@Modify list:
*              2015-5-13  xiaojun Hu   Init
*              2019-05-27 wuyan        modify
******************************************************************************/
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   //less two groups no split 
   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      println( "--least two groups" );
      return;
   }

   var csName = CHANGEDPREFIX + "_largeThanPageCS159";
   var clName = CHANGEDPREFIX + "_cl159";
   commDropCS( db, csName, true, "drop cs begin" );

   var pageSize = 4096;
   var srcGroup = groups[0][0]["GroupName"];
   var destGroup = groups[1][0]["GroupName"];
   commCreateCS( db, csName, true, "", { PageSize: pageSize } );
   var cl = createCLAndInsertData( srcGroup, csName, clName, pageSize );

   splitCL( cl, srcGroup, destGroup );

   truncateAndCheckResult( cl, csName, clName );

   commDropCS( db, csName, true, "drop cs end" );

}


function createCLAndInsertData ( srcGroup, csName, clName, pageSize )
{
   println( "---begin to create cl, than insert records" );

   var clOption = {
      "ShardingKey": { "ID_Default": 1 }, "ShardingType": "range",
      "ReplSize": 0, "Group": srcGroup
   };

   var cl = commCreateCLByOption( db, csName, clName, clOption, true,
      true, false, "create collection begin" );

   var recordNum = 4;
   var recordSize = pageSize * 4;

   // insert record And check DataPages, the TotalDataPages is 72
   truncateInsertRecord( cl, recordNum, recordSize );
   return cl;
}

function splitCL ( cl, srcGroup, destGroup )
{
   println( "---begin to split" );
   var percent = 50;
   cl.split( srcGroup, destGroup, percent );
}

function truncateAndCheckResult ( cl, csName, clName )
{
   println( "---begin to truncate,than check result" )
   cl.truncate();
   //after truncate ,the TotalDataPages is 0
   truncateVerify( db, csName + "." + clName );

   var expCount = 0;
   var actCount = cl.count();
   if( Number( expCount ) !== Number( actCount ) )
   {
      throw buildException( "truncateAndCheckResult", "truncate error!", "count",
         expCount, actCount );
   }
}