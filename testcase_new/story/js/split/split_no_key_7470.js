/******************************************************************************
@Description : 1. split range-collection by range,the condition is not shardingKey
@Modify list :
               2014-07-04  pusheng Ding  Init
               2016-02-18  wuyan changed（add the testcase describe，and add check split result ）
******************************************************************************/
function main ()
{
   try
   {
      //createCS
      var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
      //createCL , split in a
      var varCL = varCS.createCL( COMMCLNAME, {
         ShardingKey: { id: 1 }, ShardingType: "range",
         ReplSize: 0, Compressed: true
      } );
   } catch( e )
   {
      throw e;
   }
   var arrGroupName = getGroupName2( db, true );
   try
   {
      if( !( 1 < arrGroupName.length ) )
      {
         println( "least two groups" );
         throw e;
      }
   } catch( e )
   {
      return 0;
   }
   var PGname = getPG( COMMCSNAME, COMMCLNAME );
   var _PGname = PGname;
   var t = 1;
   var catadb = new Sdb( COORDHOSTNAME, CATASVCNAME );
   //SLgroupID are groupsID
   var SLgroupID = [];
   for( var i = 0; i != arrGroupName.length; ++i )
   {
      if( PGname == arrGroupName[i][0] )
      {
         SLgroupID.push( arrGroupName[i][1] + ":" + arrGroupName[i][2] );
         break;
      }
   }

   //insert data
   try
   {
      var recs = [];
      for( var i = 0; i < 50 * 2; i++ )
      {
         recs.push( { money: i } );
      }
      varCL.insert( recs );
   }
   catch( e )
   {
      throw e;
   }

   //groups split in sepecific condition
   if( 2 == arrGroupName.length )
   {

      for( var i = 0; i != arrGroupName.length; ++i )
      {
         if( PGname != arrGroupName[i][0] )
         {
            try
            {
               varCL.split( PGname, arrGroupName[i][0], { test: 50 } );
               while( catadb.SYSCAT.SYSTASKS.find().count() != 0 );
               SLgroupID.push( arrGroupName[i][1] + ":" + arrGroupName[i][2] );
            } catch( e )
            {
               throw e;
            }
            break;
         }
      }
   }
   else
   { //groups are more than two,we split two groups only
      //insert data
      for( var i = 0; i != 3 && t < 3; ++i )
      {
         if( PGname != arrGroupName[i][0] )
         {
            if( _PGname != arrGroupName[i][0] )
            {
               try
               {
                  println( PGname + "~~~~" + arrGroupName[i][0] );
                  varCL.split( PGname, arrGroupName[i][0], { test: 500 * t } );
                  while( catadb.SYSCAT.SYSTASKS.find().count() != 0 );
                  SLgroupID.push( arrGroupName[i][1] + ":" + arrGroupName[i][2] );
                  break;
               }
               catch( e )
               {
                  throw e;
               }
            }
         }
      }
   }

   //add check split result ,by wuyan 2016-2-18
   try
   {
      var gdb = new Sdb( SLgroupID[0] );
      var len = eval( "gdb." + COMMCSNAME + "." + COMMCLNAME + ".find().size()" );
      if( len != 0 )
      {
         throw buildException( "checkClSplitResult", "count wrong", "find().size()", "0", len )
      }
   }
   catch( e )
   {
      if( -34 != e )
      {
         println( "Collection does not exist , rc = " + e );
         throw e;
      }
   }

   try
   {
      var gdb = new Sdb( SLgroupID[1] );
      var len1 = eval( "gdb." + COMMCSNAME + "." + COMMCLNAME + ".find().size()" );
      if( len1 != 100 )
      {
         throw buildException( "checkClSplitResult", "count wrong", "find().size()", "100", len1 )
      }
   }
   catch( e )
   {
      throw e;
   }

}

try
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
   } else
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "drop CL in the beginning" );
      main();
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
         "drop CL in the end" );
      db.close();
   }
} catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "drop CL in the end" );
   db.close();
   throw e;
}
