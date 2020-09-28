var tmpBinData = {
   help: BinData.prototype.help,
   toString: BinData.prototype.toString
};
var funcBinData = BinData;
var funchelp = BinData.help;
BinData=function(){try{return funcBinData.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
BinData.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
BinData.prototype.help=function(){try{return tmpBinData.help.apply(this,arguments);}catch(e){commThrowError(e);}};
BinData.prototype.toString=function(){try{return tmpBinData.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
