/******************************************************************************
@Description : seqDB-34343:getDetail() 优化测试
@Modify list :
2025-11-11 Suqiang Lin Create
****************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var csName = "cs_34343";
   var pubLobFile = "lob_34343";
   var normalCLName = "normal_34343";
   var hashCLName = "hash_34343";
   var hashCLName2 = "hash_34343_2";
   var mainCLName = "main_34343";

   var cmd = new Cmd();
   var pwd = cmd.run( "pwd" ).split( "\n" )[0];
   cmd.run( "rm -rf " + pubLobFile + "*" );
   lobGenerateFile( pubLobFile, 3000 );

   var groups = commGetGroups( db );
   var groupName1 = groups[0][0]["GroupName"];
   var groupName2 = groups[1][0]["GroupName"];
   var groupID1 = groups[0][0]["GroupID"];
   var groupID2 = groups[1][0]["GroupID"];

   commDropCS( db, csName, true, "Failed to drop cs in the pre-condition." );
   var cs = db.createCS( csName );
   var normalCL = cs.createCL( normalCLName, { Group: groupName1 } );
   var hashCL = cs.createCL( hashCLName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: [ groupName1, groupName2 ] } );
   var hashCL2 = cs.createCL( hashCLName2, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: [ groupName1, groupName2 ] } );
   var mainCL = cs.createCL( mainCLName, { IsMainCL: true, ShardingKey: { date: 1 }, LobShardingKeyFormat: "YYYYMMDD" } );

   mainCL.attachCL(csName + "." + hashCLName, { LowBound: { date: "20150605" }, UpBound: { date: "20150606" } });
   mainCL.attachCL(csName + "." + hashCLName2, { LowBound: { date: "20150606" }, UpBound: { date: "20150607" } });

   var clList = [ normalCL, hashCL, mainCL ];
   for (var clItr = 0; clItr < clList.length; clItr++)
   {
      println( clItr );
      cl = clList[ clItr ];

      var id1 = cl.createLobID( "2015-06-05-16.10.33" );
      cl.putLob( pubLobFile, id1 );
      var id2 = cl.createLobID( "2015-06-06-16.10.33" );
      cl.putLob( pubLobFile, id2 );

      var cursor = cl.getDetail();
      var obj1 = {};
      while (result1 = cursor.next())
      {
         println( result1 );
         var details = result1.toObj().Details;
         obj1.Name = result1.toObj().Name;
         obj1.UniqueID = result1.toObj().UniqueID;
         obj1.CollectionSpace = result1.toObj().CollectionSpace;
         for (var i = 0; i < details.length; i++)
         {
            detail = details[i];
            for ( field in detail )
            {
               if ( field.indexOf( "Total" ) == 0 )
               {
                  if ( field in obj1 )
                  {
                     obj1[ field ] += detail[ field ];
                  }
                  else
                  {
                     obj1[ field ] = detail[ field ];
                  }
               }
               else
               {
                  obj1[ field ] = detail[ field ];
               }
            }
         }
      }

      var cursor = cl.getDetail( true );
      result2 = cursor.next();
      println( result2 );
      assert.equal( cursor.next(), null );
      var obj2 = result2.toObj();
      assert.equal( obj2.Name, obj1.Name );
      assert.equal( obj2.UniqueID, obj1.UniqueID );
      assert.equal( obj2.CollectionSpace, obj1.CollectionSpace );
      var details = obj2.Details;
      assert.equal( details.length, 1 );
      var detail = details[0];
      println( JSON.stringify( detail ) );

      assert.equal( detail.NodeName, "" );
      delete detail.NodeName;
      assert.equal( detail.GroupName, "" );
      delete detail.GroupName;
      assert.equal( detail.ID, 65535 );
      delete detail.ID;
      assert.equal( detail.LogicalID, -1 );
      delete detail.LogicalID;
      assert.equal( detail.DataCommitLSN, -1 );
      delete detail.DataCommitLSN;
      assert.equal( detail.IndexCommitLSN, -1 );
      delete detail.IndexCommitLSN;
      assert.equal( detail.LobCommitLSN, -1 );
      delete detail.LobCommitLSN;
      delete detail.ResetTimestamp;
      delete detail.CreateTime;
      delete detail.UpdateTime;
      delete detail.LobUsageRate;
      delete detail.AvgLobSize;

      delete detail.DataCommitted ;
      delete detail.IndexCommitted ;
      delete detail.LobCommitted ;

      for ( field in detail )
      {
         if ( field in obj1 && field in detail )
         {
            println( field );
            assert.equal( detail[ field ], obj1[ field ] );
         }
      }

      cl.truncate();
   }

   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
   cmd.run( "rm -rf " + pubLobFile + "*" );
}
