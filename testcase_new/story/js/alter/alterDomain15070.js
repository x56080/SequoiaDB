/************************************
*@Description: 修改AutoSplit所有属性值
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15070
**************************************/

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}


function main ()
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
   var csName = CHANGEDPREFIX + "_cs_15070";
   var clName = CHANGEDPREFIX + "_cl_15070";
   var domainName = CHANGEDPREFIX + "_domain_15070";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   var group3 = allGroupName[2];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1, group2], { AutoSplit: false } );
   db.createCS( csName, { Domain: domainName } )
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash' };
   var cl = commCreateCLByOption( db, csName, clName, clOption, true, true );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   println( "---domain alter all attribute---" );
   domain.setAttributes( { AutoSplit: true, Groups: [group1, group2, group3], AutoRebalance: true } )
   checkDomain( db, domainName, [group1, group2, group3], true, true );

   println( "---domain alter all attribute will error---" );
   domainSetAtt( domain, { AutoSplit: false, Groups: [group3], AutoRebalance: false, Name: 'test_10570' } )
   checkDomain( db, domainName, [group1, group2, group3], true, true );

   db.dropCS( csName );
   commDropDomain( db, domainName );
   println( "---end the test---" );
}

function domainSetAtt ( domain, alterOption )
{
   try
   {
      domain.setAttributes( alterOption );
      throw "ALTER_ERROR";
   }
   catch( e )
   {
      if( e !== -6 && e !== -256 )
      {
         throw buildException( "domainSetAtt", "alter domain group error", "alter domain group error", "-6||-256", e );
      }
   }
}

function checkDomain ( db, domainName, expGroups, expAutoSplit, expAutoRebalance )
{
   var domainMsg = db.listDomains( { Name: domainName } ).current().toObj();
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
      if( expGroups.indexOf( groupName ) === -1 )
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

