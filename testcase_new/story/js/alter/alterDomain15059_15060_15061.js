/************************************
*@Description: 空Domain执行addGroups/setGroups/removeGroups修改组
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15059，seqDB-15060，seqDB-15061
**************************************/

main(); 

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" ); 
      return; 
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true ); 
   if( 2 >= allGroupName.length )
   {
      println( "--least two groups" ); 
      return; 
   }
   println( "---begin test---" ); 
   var domainName = CHANGEDPREFIX + "_domain_15059"; 
   var group1 = allGroupName[0]; 
   var group2 = allGroupName[1]; 
   var group3 = allGroupName[2]; 
   
   commDropDomain( db, domainName ); 
   var domain = commCreateDomain( db, domainName, [group1] ); 
   
   println( "---domain add groups---" ); 
   domain.addGroups( {Groups:[group2, group3]} ); 
   checkDomain( db, domainName, [group1, group2, group3], undefined, undefined ); 
   
   println( "---domain remove groups---" ); 
   domain.removeGroups( {Groups:[group1]} ); 
   checkDomain( db, domainName, [group2, group3], undefined, undefined ); 
   
   println( "---domain set groups---" ); 
   domain.setGroups( {Groups:[group1]} ); 
   checkDomain( db, domainName, [group1], undefined, undefined ); 
   
   println( "---domain setAtt groups---" ); 
   domain.setGroups( {Groups:[group2]} ); 
   checkDomain( db, domainName, [group2], undefined, undefined ); 
   
   commDropDomain( db, domainName ); 
   println( "---end the test---" ); 
}

function checkDomain( db, domainName, expGroups, expAutoSplit, expAutoRebalance )
{
   var domainMsg = db.listDomains( {Name:domainName} ).current().toObj(); 
   actGroups = domainMsg.Groups; 
   actAutoSplit = domainMsg.AutoSplit; 
   actAutoRebalance = domainMsg.AutoRebalance; 
   
   if( actGroups.length !== expGroups.length )
   {
      throw buildException( "checkDomain1", "check domain group error", "check domain group error", expGroups.length, actGroups.length ); 
   }
   for( var i in expGroups )
   {
      var groupName = actGroups[i].GroupName; 
      if( expGroups.indexOf( groupName )=== -1 )
      {
         throw buildException( "checkDomain2", "check domain groupName error", "check domain groupName error", expGroups, actGroups[i].GroupName ); 
      }
   }
   
   if( actAutoSplit !== expAutoSplit )
   {
      throw buildException( "checkDomain3", "check AutoSplit error", "check AutoSplit error", expAutoSplit, actAutoSplit ); 
   }
   
   if( actAutoRebalance !== expAutoRebalance )
   {
      throw buildException( "checkDomain4", "check expAutoRebalance error", "check expAutoRebalance error", expAutoRebalance, actAutoRebalance ); 
   }
}

