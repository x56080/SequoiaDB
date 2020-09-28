var tmpRegex = {
   help: Regex.prototype.help,
   toString: Regex.prototype.toString
};
var funcRegex = Regex;
var funchelp = Regex.help;
Regex=function(){try{return funcRegex.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Regex.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Regex.prototype.help=function(){try{return tmpRegex.help.apply(this,arguments);}catch(e){commThrowError(e);}};
Regex.prototype.toString=function(){try{return tmpRegex.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
