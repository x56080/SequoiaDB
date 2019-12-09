/******************************************************************************
@Description : 1. split hash-collection by range,the same condition split two times
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
         ShardingKey: { id: 1 },
         ReplSize: 0, Compressed: true
      } );
   } catch( e )
   {
      throw e;
   }
   //var arrGroupName = getGroupName(db);
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
   var docs = [];
   try
   {
      for( var i = 0; i < 50 * 2; i++ )
      {
         docs.push( { id: i, test: "test" + i } )
      }
      varCL.insert( docs );
   }
   catch( e )
   {
      throw e;
   }

   //groups split in specific condition
   if( 2 == arrGroupName.length )
   {
      for( var i = 0; i != arrGroupName.length; ++i )
      {
         if( PGname != arrGroupName[i][0] )
         {
            try
            {
               varCL.split( PGname, arrGroupName[i][0], { id: 50 } );
               while( catadb.SYSCAT.SYSTASKS.find().count() != 0 );
               SLgroupID.push( arrGroupName[i][1] + ":" + arrGroupName[i][2] );
               //add split by overlap condition
               varCL.split( PGname, arrGroupName[i][0], { id: 50 } );
               println( "3=" + PGname )

            } catch( e )
            {
               if( -176 !== e )
                  println( "split failed: " + e );
            }
            break;
         }
      }
   }
   else
   { //groups are more than two,we split two groups only     
      for( var i = 0; i != 3 && t < 3; ++i )
      {
         if( PGname != arrGroupName[i][0] )
         {
            if( _PGname != arrGroupName[i][0] )
            {
               try
               {
                  println( PGname + "~~~~" + arrGroupName[i][0] );
                  varCL.split( _PGname, arrGroupName[i][0], { id: 50 }, { id: 100 } );
                  while( catadb.SYSCAT.SYSTASKS.find().count() != 0 );
                  PGname = arrGroupName[i][0];
                  SLgroupID.push( arrGroupName[i][1] + ":" + arrGroupName[i][2] );
                  ++t;
               } catch( e )
               {
                  if( -176 !== e )
                     println( "split failed: " + e );
                  //throw e;
               }
            }
         }
      }
   }

   var total = 0;
   for( var i = 0; i != SLgroupID.length; ++i )
   {
      var gdb = new Sdb( SLgroupID[i] );
      println( "SLgroupID:" + i + ":" + SLgroupID[i] );
      try
      {
         var len = eval( "gdb." + COMMCSNAME + "." + COMMCLNAME + ".find().size()" );
         println( len )
         total += len;
      }
      catch( e )
      {
         println( "failed to get len: " + len );
         //throw e;
      }
   }
   //add verify the result :by wuyan 2016-2-18
   if( Number( total ) !== 100 )
   {
      throw buildException( "checkHashClSplitResult()", "datas total wrong", "total count()", 100, total );
   }
   //println(catadb.SYSCAT.SYSCOLLECTIONS.find());
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
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop CL in the " );
   db.close();
   throw e;
}
