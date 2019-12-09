/******************************************************************************
*@Description : seqDB-19435:存储过程调用Decimal函数
*@author      : luweikang
*@createDate  : 2019.9.12
******************************************************************************/
main();

function main ()
{
    if( true == commIsStandalone( db ) )
    {
        println( "run mode is standalone" );
        return;
    }
    var csName = "cs19435";
    var clName = "cl19435";
    commDropCS( db, csName, true, "drop CS in the beginning" );
    commRemoveProcedure( db, "insert19435" );
    commRemoveProcedure( db, "delete19435" );

    var cl = commCreateCL( db, csName, clName, 0 );

    db.createProcedure( function insert19435 () { db.getCS( "cs19435" ).getCL( "cl19435" ).insert( { a: NumberDecimal( "100.04", [5, 2] ) } ); } );
    db.eval( "insert19435()" );

    var cursor = findData( cl, { a: NumberDecimal( "100.04", [5, 2] ) } );
    var expRecs = [{ "a": { "$decimal": "100.04", "$precision": [5, 2] } }];
    checkRec( cursor, expRecs );

    db.createProcedure( function delete19435 () { db.getCS( "cs19435" ).getCL( "cl19435" ).remove( { a: NumberDecimal( "100.04", [5, 2] ) } ); } );
    db.eval( "delete19435()" );

    cl.insert( { a: 1 } );

    var cursor = findData( cl );
    var expRecs = [{ "a": 1 }];
    checkRec( cursor, expRecs );

    db.removeProcedure( "insert19435" );
    db.removeProcedure( "delete19435" );
    commDropCS( db, csName, true, "drop CS in the end" );
}