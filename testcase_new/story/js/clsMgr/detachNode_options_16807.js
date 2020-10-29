/* *****************************************************************************
@discretion: detachNode( )options参数为空
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
         db.getRG( groupName ).detachNode( COORDHOSTNAME, RSRVPORTBEGIN );
         throw new Error( "exp fail but found success" );
      } catch( e )
      {
         if( e.message != -259 ) 
         {
            throw new Error( "detachNode with options 'test' fail" + e.message );
         }
      }
   }
   catch( e )
   {
      throw new Error( "check detachNode16807" + e.message );
   } finally
   {
      if( db !== undefined )
      {
         db.close();
      }
   }
}

