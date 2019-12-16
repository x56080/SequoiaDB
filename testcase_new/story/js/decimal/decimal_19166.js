/******************************************************************************
*@Description : test illegal special decimal value
*               seqDB-19166 : Decimal函数参数校验       
*@author      : luweikang 
******************************************************************************/
main();

function main ()
{
    commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
    var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

    var legalDocs = [{ a: NumberDecimal( "mAx" ) },
    { a: NumberDecimal( "MiN" ) },
    { a: NumberDecimal( "-Inf" ) },
    { a: NumberDecimal( "iNF" ) },
    { a: NumberDecimal( "nan" ) }];
    insertData( cl, legalDocs );

    try
    {
        var a = { a: NumberDecimal( "MAX1" ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('MAX1') }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100", [4] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100', [4]) }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100.1001", [100, 2, 1] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100.1001', [100, 2, 1]) }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100", ["a", "b"] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100', ['a', 'b']) }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100abc", [4, 1] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100abc', [4, 1]) }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal() };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal() }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100", [1.1, 1.2] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100', [1.1, 1.2]) }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100", [100, "a"] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100', [100, 'a']) }", -6, e );
        }
    }

    try
    {
        var a = { a: NumberDecimal( "100", [3, 1] ) };
        throw 0;
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "main", e, "illegal decimal = { a: NumberDecimal('100', [3, 1]) }", -6, e );
        }
    }

    commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
}