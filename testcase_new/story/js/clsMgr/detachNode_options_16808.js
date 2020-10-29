/* *****************************************************************************
@discretion: detachNode( )options参数格式非法
@author：2018-12-12 wangkexin
***************************************************************************** */
testConf.skipOneGroup = true;
main( test );
function test ()
{
   try
   {
      var groupList = getGroup( db );
      var groupName = groupList[0];

      try
      {
         db.getRG( groupName ).detachNode( COORDHOSTNAME, RSRVPORTBEGIN, "test" );
         throw new Error( "exp fail but found success" );
      } catch( e )
      {
         if( e.message != -6 ) 
         {
            throw new Error( "detachNode with options 'test' fail" + e.message );
         }
      }
   }
   catch( e )
   {
      throw new Error( "check detachNode16808" + e.message );
   } finally
   {
      if( db !== undefined )
      {
         db.close();
      }
   }
}

