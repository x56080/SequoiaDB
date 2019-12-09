/* *****************************************************************************
@discretion: attachNode( )options参数格式非法
@author：2018-12-12 wangkexin
***************************************************************************** */

main( db );
function main ( db )
{
	try
	{
		if( commGetGroupsNum( db ) < 2 )
		{
			println( "--least two groups" );
			return;
		}
		var groupList = getGroup( db );
		var groupName = groupList[0];

		try
		{
			db.getRG( groupName ).attachNode( COORDHOSTNAME, RSRVPORTBEGIN, "test" );
			throw "exp fail but found success";
		} catch( e )
		{
			if( e !== -6 ) 
			{
				throw buildException( "attachNode without options fail", e );
			}
		}
	}
	catch( e )
	{
		throw buildException( "check attachNode16804", e )
	} finally
	{
		if( db !== undefined )
		{
			db.close();
		}
	}
}

