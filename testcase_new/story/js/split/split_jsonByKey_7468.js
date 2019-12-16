/******************************************************************************
*@Description: test split collection
*@Modify list:
*              2015-5-28  xiaojun Hu   changed
******************************************************************************/

/*******************************************************************************
*@Description: 测试切分数据到不同的组上去
*@Input: collection.split()
*@Expectation: 切分成功，数据切分正确
********************************************************************************/
function testSplitJsonOne ( db )
{
   var funcName = "testSplitJsonOne";
   try
   {
      println( "" );
      var splitCLOption = {
         ShardingKey: { name: 1 }, ShardingType: "range",
         ReplSize: 0, Compressed: true
      };
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "drop cl begin" )
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, splitCLOption, true,
         true, false, "create collection begin" );
      var fullName = COMMCSNAME + "." + COMMCLNAME;
      var dstGroup;

      var json_in = [{ no: 0, first: "James", last: "Queeen" },
      { no: 1, first: "James", last: "Band" },
      { no: 2, first: "Queeen", last: "Queeen" },
      { no: 3, first: "Queeen", last: "Band" },
      { no: 4, first: "Band", last: "Queeen" }];
      var srcGroup = commGetCLGroups( db, fullName );
      var groups = commGetGroups( db );
      if( groups.length < 2 )
      {
         println( "only have " + groups.length +
            " group, expect at least 2 groups" );
         return;
      }

      // insert data
      for( var j = 0; j < json_in.length; ++j )
      {
         cl.insert( { name: json_in[j] } );
      }

      var json_cnt = 1;
      // split group
      for( var i = 0; i < groups.length; ++i )
      {
         if( srcGroup[0] == groups[i][0]["GroupName"] )
         {
            // when get source group, go next loop
            continue;
         }
         dstGroup = groups[i][0]["GroupName"];
         if( 2 == groups.length )
         {
            println( "source group: " + srcGroup[0] + ", destnation group: " +
               dstGroup + ", split key:{name:{first:'James',last:'Band'}}" );
            cl.split( srcGroup[0], dstGroup, { name: json_in[0] },
               { name: json_in[1] } );


            // verify data
            var rg = db.getRG( dstGroup );
            var db_prim = new Sdb( rg.getMaster() );
            var findRet = eval( "db_prim ." + fullName + ".find()" );
            var findRetObj = eval( "(" + findRet.toArray() + ")" );
            var expectVal = JSON.stringify( json_in[0] );
            var actualVal = JSON.stringify( findRetObj["name"] );
            if( expectVal != actualVal )
            {
               println( 'expect: {"name":' + expectVal + "}" );
               println( 'actual: {"name":' + actualVal + "}" );
               throw "split wrong";
            }
         }
         else
         {
            println( "source group: " + srcGroup[0] + ", destnation group: " +
               dstGroup + ". split key: {\"name\":" +
               JSON.stringify( json_in[json_cnt] ) + "}" );
            cl.split( srcGroup[0], dstGroup, { name: json_in[json_cnt] },
               { name: json_in[json_cnt + 1] } );
            // verify data
            var rg = db.getRG( dstGroup );
            var db_prim = new Sdb( rg.getMaster() );
            var findRet = eval( "db_prim ." + fullName + ".find()" );
            var findRetObj = eval( "(" + findRet.toArray() + ")" );
            var expectVal = JSON.stringify( json_in[json_cnt] );
            var actualVal = JSON.stringify( findRetObj["name"] );
            if( expectVal != actualVal )
            {
               println( 'expect: {"name":' + expectVal + "}" );
               println( 'actual: {"name":' + actualVal + "}" );
               throw "split wrong";
            }

            if( json_cnt < json_in.length - 1 )
            {
               json_cnt++;
            }
            else
            {
               // max split 4 groups
               break;
            }
         }
      }
   }
   catch( e )
   {
      throw buildException( funcName, e );

   }
   finally
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
         "drop cl end" )
   }
}

function main ()
{
   var funcName = "main";
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      testSplitJsonOne( db )
      println( "\n Test <testSplitJsonOne> Over" );
   }
   catch( e )
   {
      throw e;
   }
}

main();
