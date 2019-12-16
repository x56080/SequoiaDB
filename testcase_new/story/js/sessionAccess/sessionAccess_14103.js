/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid for group slave node,than query after insert data
@author��2018-1-29 wuyan  Init
***************************************************************************** */

main();

function main ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_sessionAcess14103";
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var groups = commGetGroups( db );
      var clGroupName = groups[0][0]["GroupName"];
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0, Group: clGroupName }, true, true );

      println( "---begin to set instanceid " );
      db.setSessionAttr( { PreferedInstance: "S" } )
      insertData( dbcl );
      var queryNode = getAccessNode( dbcl );
      checkAccessNodeIsPrimary( queryNode, clGroupName, true );
      println( "---end to set instanceid " );

      commDropCL( db, COMMCSNAME, clName, true, true,
         "clear collection in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

