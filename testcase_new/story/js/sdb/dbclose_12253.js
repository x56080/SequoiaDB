/******************************************************************************
*@Description : test db operation after close
*               TestLink : seqDB-12253 ����close��ִ�в���
*@auhor       : Liang XueWang
******************************************************************************/
function main ()
{
    var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
    db.close();

    try
    {
        db.traceResume();
        throw "NEED_ERROR";
    }
    catch( e )
    {
        if( e === 0 )
        {
            throw new Error( e );
        }
    }
}

try
{
    //SEQUOIADBMAINSTREAM-5230
    //main() ;
}
catch( e )
{
    if( e.constructor === Error )
    {
        println( e.stack );
    }
    throw e;
}
