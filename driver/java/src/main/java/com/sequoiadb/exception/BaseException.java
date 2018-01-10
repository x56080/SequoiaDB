/**
 *      Copyright (C) 2012 SequoiaDB Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
package com.sequoiadb.exception;

import com.sequoiadb.base.SequoiadbConstants;

import java.util.Arrays;

/**
 * @author tanzhaboo
 * 
 */
public class BaseException extends RuntimeException {

	private static final long serialVersionUID = -6115487863398926195L;

	private SDBError error;
	private String infos = "";

	private void _buildException(String errorType, int errorCode, String detail) {
		int code = 0;
		String type = "";
		String desc = "";
		if ((errorType == null || errorType.isEmpty()) && errorCode >= 0) {
			// in case no valid error info
			code = 0;
			type = SequoiadbConstants.UNKNOWN_TYPE;
			desc = SequoiadbConstants.UNKNOWN_DESC;
		} else if (errorType == null || errorType.isEmpty()) {
			// in case no error type
			code = errorCode;
			try {
				type = SDBErrorLookup.getErrorTypeByCode(code);
			} catch (Exception e1) {
				type = SequoiadbConstants.UNKNOWN_TYPE;
				desc = SequoiadbConstants.UNKNOWN_DESC;
			}
			if (type != SequoiadbConstants.UNKNOWN_TYPE) {
				try {
					desc = SDBErrorLookup.getErrorDescriptionByType(type);
				} catch (Exception e2) {
					desc = SequoiadbConstants.UNKNOWN_DESC;
				}
			}
		} else if (errorCode >= 0) {
			type = errorType;
			try {
				code = SDBErrorLookup.getErrorCodeByType(type);
			} catch (Exception e2) {
				code = 0;
			}
			try {
				desc = SDBErrorLookup.getErrorDescriptionByType(type);
			} catch (Exception e2) {
				desc = SequoiadbConstants.UNKNOWN_DESC;
			}
		}
		// build error detail
		infos = String.format("%s(%d): %s, detail: %s", type, code, desc, detail);

		error = new SDBError();
		error.setErrorType(type);
		error.setErrorCode(code);
		error.setErrorDescription(infos);
	}

	private BaseException(String errorType, int errorCode, String detail) {
		_buildException(errorType, errorCode, detail);
	}

	private BaseException(String errorType, int errorCode, String detail, Throwable e) {
		super(e);
		_buildException(errorType, errorCode, detail);
	}

	/**
	 *
	 * @param errorType
	 * @param detail
	 * @param e
	 */
	public BaseException(String errorType, String detail, Throwable e) {
		this(errorType, 0, detail, e);
	}

	/**
	 *
	 * @param errorType
	 * @param e
	 */
	public BaseException(String errorType, Throwable e) {
		this(errorType, e.getMessage(), e);
	}

	/**
	 *
	 * @param errorType
	 * @param detail
	 */
	public BaseException(String errorType, String detail) {
		this(errorType, 0, detail);
	}

	/**
	 *
	 * @param errorType
	 */
	public BaseException(String errorType) {
		this(errorType, "");
	}

//	public BaseException(int errorCode, String detail, Throwable e) {
//		this(null, errorCode, detail, e);
//	}
//
//	public BaseException(int errorCode, Throwable e) {
//		this(errorCode, null, e);
//	}
//
//	public BaseException(int errorCode, String detail) {
//		this(null, errorCode, detail);
//	}

	/**
	 *
	 * @param errorCode
	 */
	public BaseException(int errorCode) {
		this(null, errorCode, null);
	}

	/**
	 * @param errorType
	 * @throws Exception
	 * @deprecated
	 */
	public BaseException(String errorType, Object... info) {
		this(errorType, 0, Arrays.toString(info));
	}

	/**
	 * 
	 * @param errorCode
	 * @throws Exception
	 * @deprecated
	 */
	public BaseException(int errorCode, Object... info) {
		this(null, errorCode, Arrays.toString(info));
	}

	@Override
	public String getMessage() {
		return error.getErrorDescription();
	}

	public String getErrorType() {
		return error.getErrorType();
	}

	public int getErrorCode() {
		return error.getErrorCode();
	}
}
