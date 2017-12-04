package com.sequoiadb.datasource;

import org.testng.annotations.DataProvider;
import com.sequoiadb.base.SequoiadbOption;
import com.sequoiadb.datasource.ConnectStrategy;

public class SdbTestOptionFactory {
	@DataProvider(name = "option-provider")
	public static Object[][] create(){
		return new Object[][]{
				{createSequoiadbOption(ConnectStrategy.LOCAL)},
				{createSequoiadbOption(ConnectStrategy.SERIAL)},
				{createSequoiadbOption(ConnectStrategy.RANDOM)},
				{setDisablesyncCoord(createSequoiadbOption(ConnectStrategy.BALANCE))},
				{setsyncCoordInterval(createSequoiadbOption(ConnectStrategy.BALANCE))}
		};
	}
	
	private static SequoiadbOption setsyncCoordInterval(SequoiadbOption option){
		option.setSyncCoordInterval(100);
		return option;
	}
	
	private static SequoiadbOption setDisablesyncCoord(SequoiadbOption option){
		option.setSyncCoordInterval(0);
		return option;
	}
	
	private static SequoiadbOption setValidateConnection(SequoiadbOption option){
		option.setValidateConnection(true);
		return option;
	}
	private static SequoiadbOption createSequoiadbOption(ConnectStrategy strategy){
		SequoiadbOption option =  new SequoiadbOption();
		option.setConnectStrategy(strategy);
		return option;
	}

}
