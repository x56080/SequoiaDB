/* *****************************************************************************
@discretion: attachNode( )options参数格式非法
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
         db.getRG( groupName ).attachNode( COORDHOSTNAME, RSRVPORTBEGIN, "test" );
         throw new Error( "exp fail but found success" );
      } catch( e )
      {
         if( e.message != -6 ) 
         {
            throw new Error( "attachNode without options fail" + e.message );
         }
      }
   }
   catch( e )
   {
      throw new Error( "check attachNode16804" + e.message );
   } finally
   {
      if( db !== undefined )
      {
         db.close();
      }
   }
}

