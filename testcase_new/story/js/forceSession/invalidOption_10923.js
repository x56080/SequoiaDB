/***********************************************************************
*@Description : test force session with invalid option
*               seqDB-10923:options参数非法校验
*@author      : Liang XueWang 
***********************************************************************/
function testInvalidOption ()
{
    // get session id to force
    var sessionID = db.list( 3, { Global: false } ).next().toObj().SessionID;

    // invalid option key value
    var errKeys = ["GroupID", "GroupName", "NodeID", "HostName",
        "svcname", "NodeSelect", "Role"];
    var errVals = ["", "abcd", 123];
    var errno = [[-6, -6, -154], [-154, -154, -6], [-6, -6, -155],
    [-155, -155, -6], [-155, -155, -6], [-6, -6, -6],
    [-6, -6, -6]];
    var option = {};

    for( var i = 0; i < errKeys.length; i++ )
    {
        for( var j = 0; j < errVals.length; j++ )
        {
            var key = errKeys[i];
            var val = errVals[j];
            var err = errno[i][j];
            option[key] = val;
            try
            {
                db.forceSession( sessionID, option );
                throw buildException( "testInvalidOption", null,
                    "force session with option " + key + ": " + val, err, 0 );
            }
            catch( e )
            {
                if( e !== err )
                {
                    throw buildException( "testInvalidOption", e,
                        "force session with option " + key + ": " + val,
                        err, e );
                }
                option = {};
            }
        }
    }
}

function main ()
{
    testInvalidOption();
}

main();