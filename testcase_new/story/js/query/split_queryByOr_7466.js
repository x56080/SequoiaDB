/******************************************************************************
*@Description : 1.SEQUOIADBMAINSTREAM-274:
*               range-cl is queried error when use operator OR and after split
*
*@Modify list :
*               2014-07-17 pusheng Ding  Init
******************************************************************************/
function main ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      println( "--least two groups" );
      return;
   }

   //create CL
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName;
   var destGroupName = groups[1][0].GroupName;
   var clName = COMMCLNAME + "_7466";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { b: 1 }, ShardingType: 'range', ReplSize: 0, Group: srcGroupName }, true );

   //insert data
   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      docs.push( { a: i - 10000, b: i, c: "abcdefghijkl" + i } );
   }
   dbcl.insert( docs );

   //create index
   dbcl.createIndex( "idx_7466", { a: 1 }, false, false );

   //split
   dbcl.split( srcGroupName, destGroupName, 50 );

   //select * from ... where a<=-9000 or a>=-1000 or b=2000
   try
   {
      var cursor = dbcl.find( { $or: [{ a: { $lte: -9000 } }, { a: { $gte: -1000 } }, { b: 2000 }] } ).sort( { a: 1 } );
      var count = 0;
      var expected = 2002;
      while( cursor.next() )
      {
         count++;
         var ret = cursor.current();
         if( !( ret.toObj()['a'] <= -9000 || ret.toObj()['a'] >= -1000 || ret.toObj()['b'] == 2000 ) )
         {
            println( "return incorrect record! " + ret );
            throw "check result fail";
         }
      }
      if( count != expected )
      {
         println( "return rows not expected! expected:" + expected + " return:" + count + ( count > expected ? " or more" : "" ) );
         throw "check result fail";
      }
   }
   finally
   {
      cursor.close();
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
main();
