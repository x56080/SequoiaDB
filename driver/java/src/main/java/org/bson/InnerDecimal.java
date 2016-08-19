// InnerDecimal.java

/**
 *      Copyright (C) 2008 10gen Inc.
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

package org.bson;

import org.bson.types.BSONDecimal;

public class InnerDecimal {

	public static final int DECIMAL_SIGN_MASK = 0xC000;
	public static final int SDB_DECIMAL_POS = 0x0000;
	public static final int SDB_DECIMAL_NEG = 0x4000;
	public static final int SDB_DECIMAL_SPECIAL_SIGN = 0xC000;

	public static final int SDB_DECIMAL_SPECIAL_NAN = 0x0000;
	public static final int SDB_DECIMAL_SPECIAL_MIN = 0x0001;
	public static final int SDB_DECIMAL_SPECIAL_MAX = 0x0002;

	public static final int DECIMAL_DSCALE_MASK = 0x3FFF;
	/* size + typemod + dscale + weight */
	public static final int DECIMAL_HEADER_SIZE = 12;

	// static final int SDB_DECIMAL_DBL_DIG = ( DBL_DIG );

	/*
	 * Hardcoded precision limit - arbitrary, but must be small enough that
	 * dscale values will fit in 14 bits.
	 */
	static final int DECIMAL_MAX_PRECISION = 1000;
	static final int DECIMAL_MAX_DISPLAY_SCALE = DECIMAL_MAX_PRECISION;
	static final int DECIMAL_MIN_DISPLAY_SCALE = 0;
	static final int DECIMAL_MIN_SIG_DIGITS = 16;

	static final int DECIMAL_NBASE = 10000;
	static final int DECIMAL_HALF_NBASE = 5000;
	static final int DECIMAL_DEC_DIGITS = 4; /* decimal digits per NBASE digit */
	static final int DECIMAL_MUL_GUARD_DIGITS = 2; /*
													 * these are measured in
													 * NBASE digits
													 */
	static final int DECIMAL_DIV_GUARD_DIGITS = 4;

	final int[] round_powers = { 0, 1000, 100, 10 };
	// // values for raw decimal data
	// private int _rawSize; /* total byte size of decimal data after encoding
	// */
	// private int _rawTypeMod; /*
	// * precision + scale, precision = (typemod >> 16) &
	// * 0xffff, scale = typemod & 0xffff
	// */
	// private short _rawDscale; /*
	// * sign + dscale, sign = dscale & 0xc000, scale =
	// * dscale & 0x3fff
	// */
	// private short _rawWeight; /* weigh of the decimal(NBASE=10000) */
	// private short[] _rawDigits; /* read data */

	// values for decimal for calculating
	private int _typemod; /*
						 * precision & scale define: precision = (typmod >> 16)
						 * & 0xffff scale = typmod & 0xffff
						 */
	private int _ndigits; /* length of digits */
	private int _sign; /* the decimal's sign */
	private int _dscale; /* display scale */
	private int _weight; /* weight of first digit */
	private short[] _digits; /*
							 * the decimal data start from position 1 but not
							 * position 0, we leave position 0 for carry when
							 * rounding happen
							 */
	private int _digits_idx; /* index for _digits */
	private boolean _hasCarry = false;

	// values for input decimal info
	private String _rawStrData;

	// get decimal info for calculating
	public int getTypeMod() {
		// when input nan/max/min, _digits is null
		return  (_digits != null && _digits.length > 0) ? _typemod : -1;
	}

	public int getNDigits() {
		return _ndigits;
	}

	public int getSign() {
		return _sign;
	}

	public int getDScale() {
		return _dscale;
	}

	public int getDScaleWithSign() {
		return (_dscale & DECIMAL_DSCALE_MASK) | _sign;
	}
	
	public int getWeight() {
		return _weight;
	}

	public short[] getDigits() {
		short[] retDigits = new short[_ndigits];
		// when input nan/max/min, _digits is null
		if (_digits != null && _digits.length > 0) {
			if (!_hasCarry) {
				System.arraycopy(_digits, 1, retDigits, 0, _ndigits);
			} else {
				System.arraycopy(_digits, 0, retDigits, 0, _ndigits);
			}
		}
		return retDigits;
	}

	public void setTypeMod(int typemod) {
		_typemod = typemod;
	}

	public void setNDigits(int ndigits) {
		_ndigits = ndigits;
	}

	public void setSign(int sign) {
		_sign = sign;
	}

	public void setDScale(int dscale) {
		_dscale = dscale;
	}

	public void setWeight(int weight) {
		_weight = weight;
	}

	public void setDigits(short[] digits) {
		_digits = new short[digits.length];
		System.arraycopy(digits, 0, _digits, 0, digits.length);
	}

	public InnerDecimal() {
	}

	/**
	 * @fn size()
	 * @brief return the total bytes for saving decimal
	 * @return
	 * 
	 */
	public int size() {
		return DECIMAL_HEADER_SIZE + _ndigits * (Short.SIZE / 8);
	}

	public void fromBSONDecimal(BSONDecimal decimal) {
		boolean have_dp = false;
		int sign = SDB_DECIMAL_POS;
		int dweight = -1;
		int ddigits = 0;
		int dscale = 0;
		int weight = -1;
		int ndigits = 0;
		int offset = 0;

		_getDecimalInfo(decimal);

		char[] cp = _rawStrData.toCharArray();
		int cp_idx = 0;

		if (_rawStrData.length() >= 3) {
			// not a number
			if ((cp[0] == 'n' || cp[0] == 'N')
					&& (cp[1] == 'a' || cp[1] == 'A')
					&& (cp[2] == 'n' || cp[2] == 'N')) {
				_setNan();
				return;
			}
			// min
			if ((cp[0] == 'm' || cp[0] == 'M')
					&& (cp[1] == 'i' || cp[1] == 'I')
					&& (cp[2] == 'n' || cp[2] == 'N')) {
				_setMin();
				return;
			}
			// max
			if ((cp[0] == 'm' || cp[0] == 'M')
					&& (cp[1] == 'a' || cp[1] == 'A')
					&& (cp[2] == 'x' || cp[2] == 'X')) {
				_setMax();
				return;
			}
		}

		/*
		 * We first parse the string to extract decimal digits and determine the
		 * correct decimal weight. Then convert to NBASE representation.
		 */
		switch (cp[cp_idx]) {
		case '+':
			sign = SDB_DECIMAL_POS;
			cp_idx++;
			break;

		case '-':
			sign = SDB_DECIMAL_NEG;
			cp_idx++;
			break;
		}

		if (cp[cp_idx] == '.') {
			have_dp = true;
			cp_idx++;
		}

		if (!Character.isDigit(cp[cp_idx])) {
			throw new IllegalArgumentException("invalid decimal: "
					+ _rawStrData);
		}
		// actually, "decdigitsnum" is more than what we need, we need to make
		// sure
		// we have enough space for holding the digits
		int decdigits_idx = 0;
		int decdigitsnum = (cp.length - cp_idx) + DECIMAL_DEC_DIGITS * 2;
		int[] decdigits = new int[decdigitsnum];
		decdigits_idx = DECIMAL_DEC_DIGITS;

		while (cp_idx < cp.length) {
			if (Character.isDigit(cp[cp_idx])) {
				decdigits[decdigits_idx++] = Character.digit(cp[cp_idx++], 10);
				if (!have_dp) {
					dweight++;
				} else {
					dscale++;
				}
			} else if (cp[cp_idx] == '.') {
				if (have_dp) {
					throw new IllegalArgumentException(
							"invalid decimal point in decimal: " + _rawStrData);
				} else {
					have_dp = true;
					cp_idx++;
				}
			} else {
				if (cp[cp_idx] != 'e' && cp[cp_idx] != 'E') {
					// invalid argument
					throw new IllegalArgumentException(
							"invalid digit in decimal: " + _rawStrData);
				} else {
					// when cp[cp_idx] is 'e' or 'E', let's handle it later
					break;
				}
			}
		}

		// "ddigits" does not contain '.'
		ddigits = decdigits_idx - DECIMAL_DEC_DIGITS;

		/* Handle exponet, if any */
		if ((cp_idx < cp.length) && (cp[cp_idx] == 'e' || cp[cp_idx] == 'E')) {
			long exponent = 0L;
			cp_idx++;
			String strexponent = String.valueOf(cp, cp_idx, cp.length - cp_idx);
			try {
				exponent = Long.parseLong(strexponent);
			} catch (NumberFormatException e) {
				throw new IllegalArgumentException(
						"invalid exponet in decimal: " + _rawStrData);
			}
			if (exponent > DECIMAL_MAX_PRECISION
					|| exponent < -DECIMAL_MAX_PRECISION) {
				throw new IllegalArgumentException("exponet in decimal: "
						+ _rawStrData + " is out of bound");
			}
			dweight += (int) exponent;
			dscale -= (int) exponent;
			if (dscale < 0) {
				dscale = 0;
			}
		}

		/*
		 * Okay, convert pure-decimal representation to base NBASE. First we
		 * need to determine the converted weight and ndigits. offset is the
		 * number of decimal zeroes to insert before the first given digit to
		 * have a correctly aligned first NBASE digit.
		 */
		if (dweight >= 0) {
			// "dweight + 1" is the count of digits in the left of decimal
			// point,
			// "(dweight + 1 + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS"
			// tells
			// us which part is the first digit in,
			// because when dweight >= 0, weight starts from 0(the first part in
			// the
			// left of decimal point), so we need to decrease 1
			weight = (dweight + 1 + DECIMAL_DEC_DIGITS - 1)
					/ DECIMAL_DEC_DIGITS - 1;
		} else {
			// "-dweight - 1" means how many "0" exists ahead the first valid
			// digit,
			// for example, "0.00000123", "dweight" is -6, "-dweight - 1" means
			// there
			// has 5 "0" ahead the first valid digit "1".
			// "(-dweight - 1)/DECIMAL_DEC_DIGITS + 1" tells us which part is
			// the
			// first valid digit in, because when dweight < 0, weight starts
			// from -1(
			// the first part in the right of decimal point), so we need to
			// increase 1
			weight = -((-dweight - 1) / DECIMAL_DEC_DIGITS + 1);
		}
		// notic that: when we input a decimal in format of ".123456" or
		// ".01234e5"
		// "dweight","weight" and "offset" do not express their own meaning
		// accurately, but function "_decimal_strip" will help us to fix the
		// "weight", and what we need to care about is the corrent "weight".

		offset = (weight + 1) * DECIMAL_DEC_DIGITS - (dweight + 1);
		ndigits = (ddigits + offset + DECIMAL_DEC_DIGITS - 1)
				/ DECIMAL_DEC_DIGITS;

		// alloc buffer for keeping digits
		_alloc(ndigits);

		// keep result to local
		_sign = sign;
		_weight = weight;
		_dscale = dscale;

		decdigits_idx = DECIMAL_DEC_DIGITS - offset;
		while (ndigits-- > 0) {
			_digits[_digits_idx++] = (short) (((decdigits[decdigits_idx] * 10 + decdigits[decdigits_idx + 1]) * 10 + decdigits[decdigits_idx + 2]) * 10 + decdigits[decdigits_idx + 3]);
			decdigits_idx += DECIMAL_DEC_DIGITS;
		}

		/* Strip any leading/trailing zeroes, and normalize weight if zero */
		_strip();

		_apply_typemod();

	}

	public BSONDecimal toBSONDecimal() {
		BSONDecimal retDecimal = null;
		short dig = 0;
		short d1 = 0;
//		int dscale = 0;
		int d = 0;
		int precision = _getPrecision();
		int scale = _getScale();
		String strData = null;

		int expect_char_size = _getExpectCharSize();
		char[] cp = new char[expect_char_size];
		int cp_idx = 0;
		int cp_idx_end = 0;
		// decimal is nan
		if (isNan()) {
			cp[0] = 'N';
			cp[1] = 'a';
			cp[2] = 'N';
			strData = new String(cp);
			retDecimal = new BSONDecimal(strData, precision, scale);
			return retDecimal;
		}
		// decimal is min
		if (isMin()) {
			cp[0] = 'M';
			cp[1] = 'I';
			cp[2] = 'N';
			strData = new String(cp);
			retDecimal = new BSONDecimal(strData, precision, scale);
			return retDecimal;
		}
		// decimal is max
		if (isMax()) {
			cp[0] = 'M';
			cp[1] = 'A';
			cp[2] = 'X';
			strData = new String(cp);
			retDecimal = new BSONDecimal(strData, precision, scale);
			return retDecimal;
		}
		// output a dash for negative values
		if (_sign == SDB_DECIMAL_NEG) {
			cp[cp_idx++] = '-';
		}
		// output all digits before the decimal point
		if (_weight < 0) {
			d = _weight + 1;
			cp[cp_idx++] = '0';
		} else {
			for (d = 0; d <= _weight; d++) {
				// for _digits[0] is placed carry, so we need to start from position 1
				dig = ((d < _ndigits) ? _digits[1 + d] : 0);
				// in the first digit, suppress extra leading decimal zeroes
				{
					boolean putit = (d > 0);

					d1 = (short) (dig / 1000);
					dig -= d1 * 1000;
					putit |= (d1 > 0);
					if (putit) {
						cp[cp_idx++] = (char) (d1 + '0');
					}

					d1 = (short) (dig / 100);
					dig -= d1 * 100;
					putit |= (d1 > 0);
					if (putit) {
						cp[cp_idx++] = (char) (d1 + '0');
					}

					d1 = (short) (dig / 10);
					dig -= d1 * 10;
					putit |= (d1 > 0);
					if (putit) {
						cp[cp_idx++] = (char) (d1 + '0');
					}

					cp[cp_idx++] = (char) (dig + '0');
				}
			}
		}
		/*
		 * If requested, output a decimal point and all the digits that follow
		 * it. We initially put out a multiple of DEC_DIGITS digits, then
		 * truncate if needed.
		 */
		if (_dscale > 0) {
			cp[cp_idx++] = '.';
			cp_idx_end = cp_idx + _dscale;
			for (int i = 0; i < _dscale; d++, i += DECIMAL_DEC_DIGITS) {
				// for _digits[0] is placed carry, so we need to start from position 1
				dig = (d >= 0 && d < _ndigits) ? _digits[1 + d] : 0;
				d1 = (short)(dig / 1000);
				dig -= d1 * 1000;
				cp[cp_idx++] = (char)(d1 + '0');
				
				d1 = (short)(dig / 100);
				dig -= d1 * 100;
				cp[cp_idx++] = (char)(d1 + '0');
				
				d1 = (short)(dig / 10);
				dig -= d1 * 10;
				cp[cp_idx++] = (char)(d1 + '0');
				
				cp[cp_idx++] = (char)(dig + '0');
			}
			// we need to add some zeros to tail sometimes
			while(cp_idx < cp_idx_end) {
				cp[cp_idx++] = '0';
			}
			// we don't need the extra zeros in the tail
			while(cp_idx_end < cp_idx) {
				cp[cp_idx_end] = (char)(cp[cp_idx_end] - '0');
				cp_idx_end++;
			}
		}
		// build the return BSONDecimal
		strData = new String(cp).trim();
		retDecimal = new BSONDecimal(strData, precision, scale);
		return retDecimal;
	}

	private int _getPrecision() {
		if (_typemod != -1) {
			return (_typemod >> 16) & 0xffff;
		} else {
			return -1;
		}
	}

	private int _getScale() {
		if (_typemod != -1) {
			return _typemod & 0xffff;
		} else {
			return -1;
		}
	}

	private boolean isNan() {
		if (_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _dscale == SDB_DECIMAL_SPECIAL_NAN) {
			return true;
		}
		return false;
	}

	private boolean isMin() {
		if (_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _dscale == SDB_DECIMAL_SPECIAL_MIN) {
			return true;
		}
		return false;
	}

	private boolean isMax() {
		if (_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _dscale == SDB_DECIMAL_SPECIAL_MAX) {
			return true;
		}
		return false;
	}

	private int _getExpectCharSize() {
		int retSize = 0;
		if (_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _dscale == SDB_DECIMAL_SPECIAL_NAN) {
			return (3 + 1); // "NAN" + 1
		}

		if (_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _dscale == SDB_DECIMAL_SPECIAL_MIN) {
			return (3 + 1); // "MIN" + 1
		}

		if (_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _dscale == SDB_DECIMAL_SPECIAL_MAX) {
			return (3 + 1); // "MAX" + 1
		}

		/*
		 * Allocate space for the result.
		 * 
		 * tmpSize is set to the # of decimal digits before decimal point.
		 * dscale is the # of decimal digits we will print after decimal point.
		 * We may generate as many as DEC_DIGITS-1 excess digits at the end, and
		 * in addition we need room for sign, decimal point, null terminator.
		 */
		retSize = (_weight + 1) * DECIMAL_DEC_DIGITS;
		if (retSize <= 0) {
			retSize = 1;
		}

		retSize += _dscale + DECIMAL_DEC_DIGITS + 2;

		return retSize;
	}

	private void _getDecimalInfo(BSONDecimal decimal) {
		// get info
		_rawStrData = decimal.getValue();
		int precision = decimal.getPrecision();
		int scale = decimal.getScale();
		// check
		if (precision < 0 && precision != -1 || scale < 0 && scale != -1) {
			throw new IllegalArgumentException("invalid precision: "
					+ precision + ", and scale: " + scale);
		}
		if (precision == 0 || precision > 1000 || scale > precision) {
			throw new IllegalArgumentException("invalid precision: "
					+ precision + ", and scale: " + scale);
		}
		// init
		if (precision == -1 && scale == -1) {
			_typemod = -1;
		} else {
			_typemod = ((precision << 16) | scale);
		}
		_ndigits = 0;
		_sign = SDB_DECIMAL_POS;
		_dscale = 0;
		_weight = 0;
	}

	private void _setNan() {
		_ndigits = 0;
		_weight = 0;
		_sign = SDB_DECIMAL_SPECIAL_SIGN;
		_dscale = SDB_DECIMAL_SPECIAL_NAN;
	}

	private void _setMin() {
		_ndigits = 0;
		_weight = 0;
		_sign = SDB_DECIMAL_SPECIAL_SIGN;
		_dscale = SDB_DECIMAL_SPECIAL_MIN;
	}

	private void _setMax() {
		_ndigits = 0;
		_weight = 0;
		_sign = SDB_DECIMAL_SPECIAL_SIGN;
		_dscale = SDB_DECIMAL_SPECIAL_MAX;
	}

	private void _alloc(int ndigits) {
		// we need another short for carry,
		// when rounding happen, we may use this short
		_digits = new short[ndigits + 1]; 
		_digits_idx = 0;
		_digits[_digits_idx++] = 0;
		_ndigits = ndigits;
	}

	private void _strip() {
		int ndigits = _ndigits;
		int digits_idx_f = 1;
		// _digits starts from index 1, so we need to add 1
		int digits_idx_e = (ndigits + 1) - 1;

		// strip leading zeroes
		while (ndigits > 0 && _digits[digits_idx_f] == 0) {
			digits_idx_f++;
			_weight--;
			ndigits--;
		}

		// strip trailing zeroes
		while (ndigits > 0 && _digits[digits_idx_e] == 0) {
			digits_idx_e--;
			ndigits--;
		}

		int count = digits_idx_e - digits_idx_f + 1;
		if (count != ndigits) {
			throw new IllegalStateException("ndigits[" + ndigits
					+ "] is not equal with [" + count + "]");
		}

		// if it's zero, normalize the sign and weight
		if (ndigits == 0) {
			_sign = SDB_DECIMAL_POS;
			_weight = 0;
		}

		// update the local results
		for (int i = digits_idx_f, begin_idx = 1; i <= digits_idx_e;) {
			_digits[begin_idx++] = _digits[i++]; 
		}
		for (int i = digits_idx_e + 1; i < _digits.length; i++) {
			_digits[i] = 0;
		}
		_digits_idx = 1;
		_ndigits = ndigits;
	}

	private void _apply_typemod() {
		int precision = 0;
		int scale = 0;
		int maxdigits = 0;
		int ddigits = 0;
		int i = 0;

		// Do nothing if we have a default _typemod(-1)
		if (_typemod == -1) {
			return;
		}

		precision = (_typemod >> 16) & 0xffff;
		scale = _typemod & 0xffff;
		maxdigits = precision - scale;

		// Round to target scale (and set _dscale)
		_round(scale);

		/*
		 * Check for overflow - note we can't do this before rounding, because
		 * rounding could raise the weight. Also note that the var's weight
		 * could be inflated by leading zeroes, which will be stripped before
		 * storage but perhaps might not have been yet. In any case, we must
		 * recognize a true zero, whose weight doesn't mean anything.
		 */
		ddigits = (_weight + 1) * DECIMAL_DEC_DIGITS;
		if (ddigits > maxdigits) {
			// Determine true weight; and check for all-zero result
			for (i = 0; i < _ndigits; i++) {
				short dig = _digits[i];
				if (dig != 0) {
					// Adjust for any high-order decimal zero digits
					if (dig < 10) {
						ddigits -= 3;
					} else if (dig < 100) {
						ddigits -= 2;
					} else if (dig < 1000) {
						ddigits -= 1;
					}

					if (ddigits > maxdigits) {
						throw new IllegalArgumentException(
								"the input dicimal[" + _rawStrData + "] can't be " + 
								"expressed in the range which is delimited by " +
								"the given precision[" + precision + "] and scale[" + scale + "]");
					}
					break;
				}
				ddigits -= DECIMAL_DEC_DIGITS;
			}
		}
	}

	private void _round(int rscale) {
		int di = 0;
		int ndigits = 0;
		int carry = 0;

		_dscale = rscale;

		// decimal digits wanted
		di = (_weight + 1) * DECIMAL_DEC_DIGITS + rscale;

		/*
		 * If di = 0, the value loses all digits, but could round up to 1 if its
		 * first extra digit is >= 5. If di < 0 the result must be 0.
		 */
		if (di < 0) {
			_ndigits = 0;
			_weight = 0;
			_sign = SDB_DECIMAL_POS;
		} else {
			// NBASE digits wanted
			ndigits = (di + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS;

			// 0, or number of decimal digits to keep in last NBASE digit
			di %= DECIMAL_DEC_DIGITS;

			if ((ndigits < _ndigits) || (ndigits == _ndigits && di > 0)) {
				_ndigits = ndigits;
				if (di == 0) {
					// for digits in _digits starts from position 1 but not 0,
					// so we need to increase 1
					carry = (_digits[ndigits + 1] >= DECIMAL_HALF_NBASE) ? 1
							: 0;
				} else {
					// Must round within last NBASE digit
					int extra = 0;
					int pow10 = 0;
					pow10 = round_powers[di];

					extra = _digits[--ndigits + 1] % pow10;
					_digits[ndigits + 1] -= extra;
					carry = 0;
					if (extra >= pow10 / 2) {
						pow10 += _digits[ndigits + 1];
						if (pow10 >= DECIMAL_NBASE) {
							pow10 -= DECIMAL_NBASE;
							carry = 1;
						}
						_digits[ndigits + 1] = (short) pow10;
					}
				}

				// Propagate carry if needed
				while (carry != 0) {
					carry += _digits[--ndigits + 1];
					if (carry >= DECIMAL_NBASE) {
						_digits[ndigits + 1] = (short) (carry - DECIMAL_NBASE);
						carry = 1;
					} else {
						_digits[ndigits + 1] = (short) carry;
						carry = 0;
					}
				}

				if (ndigits < 0) {
					// better not have added > 1 digit
					if (ndigits != -1) {
						throw new IllegalStateException("ndigits[" + ndigits
								+ "] is not -1");
					}
					if (_hasCarry) {
						throw new IllegalStateException(
								"impossible for _hasCarry to be true");
					}
					if (_digits[0] == 0) {
						throw new IllegalStateException(
								"impossible for _digits[0] to be 0");
					}
					_hasCarry = true;
					_ndigits++;
					_weight++;
				}
			}
		}
	}

}
