/******************************************************************************
*@Description : Public function for testing split.
*@Modify list :
*               2014-6-17  xiaojun Hu  Init
******************************************************************************/
import( "../lib/basic_operation/sequoiadb.js" )
import( "../lib/main.js" )

/* ***************************************************
@Description : check cl groups and record number
@Modify list : XiaoNi Huang 2019-12-20
*************************************************** */
function checkSplitResults ( csName, clName, expGroupNames, expTotalRecsNum )
{
   // check cl groups
   var expCLGroups = expGroupNames.sort().toString();
   var actCLGroups = commGetCLGroups( db, csName + "." + clName ).sort().toString();
   if( actCLGroups !== expCLGroups )
   {
      throw new Error( "expCLGroups = " + expCLGroups + ", actCLGroups = " + actCLGroups );
   }

   // check record number
   var actTotalRecsNum = 0;
   for( var i = 0; i < expGroupNames.length; i++ ) 
   {
      var nodeDB = db.getRG( expGroupNames[i] ).getMaster().connect();
      var cnt = nodeDB.getCS( csName ).getCL( clName ).count();
      actTotalRecsNum += Number( cnt );
   }

   if( actTotalRecsNum !== expTotalRecsNum )
   {
      throw new Error( "expTotalRecsNum = " + expTotalRecsNum + ", actTotalRecsNum = " + actTotalRecsNum );
   }
}

/* ***************************************************
@Description : get dstGroupName for split
@Modify list : XiaoNi Huang 2019-12-31
*************************************************** */
function getDstGroupName ( exceptGroupName )
{
   if( exceptGroupName === undefined )
   {
      throw new Error( "missing parameter exceptGroupName." );
   }
   var groupNames = commGetDataGroupNames( db );
   for( var i = 0; i < groupNames.length; i++ )
   {
      if( groupNames[i] !== exceptGroupName )
      {
         return groupNames[i];
      }
   }
}