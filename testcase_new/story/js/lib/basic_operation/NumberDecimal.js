var tmpNumberDecimal = {
   help: NumberDecimal.prototype.help,
   toString: NumberDecimal.prototype.toString,
   valueOf: NumberDecimal.prototype.valueOf
};
var funcNumberDecimal = NumberDecimal;
var funchelp = NumberDecimal.help;
NumberDecimal=function(){try{return funcNumberDecimal.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
NumberDecimal.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
NumberDecimal.prototype.help=function(){try{return tmpNumberDecimal.help.apply(this,arguments);}catch(e){commThrowError(e);}};
NumberDecimal.prototype.toString=function(){try{return tmpNumberDecimal.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
NumberDecimal.prototype.valueOf=function(){try{return tmpNumberDecimal.valueOf.apply(this,arguments);}catch(e){commThrowError(e);}};
