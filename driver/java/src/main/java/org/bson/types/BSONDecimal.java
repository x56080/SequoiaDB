// BSONTDecimal.java

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

package org.bson.types;

import java.io.Serializable;
import java.math.BigDecimal;

import org.bson.util.JSON;

/**
 * The type <code>BSONDecimal</code> object can store numbers with a very large
 * number of digits.
 * <p>
 * We use the following terms below: The <code>scale</code> of a
 * <code>BSONDecimal</code> is the max number of digits after the decimal point. 
 * The <code>precision</code> of a <code>BSONDecimal</code> is the total count 
 * of significant digits in the whole number,
 * that is, the number of digits to both sides of the decimal point. So the
 * number 23.5141 has a <code>precision</code> of 6 and a <code>scale</code> of
 * 4. Integers can be considered to have a <code>scale</code> of zero. Besides,
 * when user does not specified <code>precision</code> and <code>scale</code> to
 * build a <code>BSONDecimal</code> object, set both of them as -1. That means
 * the <code>precision</code> and <code>scale</code> are determined by the upper
 * limit of accuracy of the database. Database allow 131072 digits before decimal 
 * point and 16383 digits after decimal point at most.
 * </p>
 * <p>
 * By the way, when user specified the <code>precision</code> and <code>scale</code>, 
 * if the fractional part of the offered decimal is greater the
 * <code>scale</code>, BSONDecimal will round the fractional part to the
 * specified <code>scale</code> with the mode of HALP_UP.
 * </p>
 * Here is a example:
 * 
 * <pre>
 * BSONDecimal decimal1 = new BSONDecimal(&quot;123.456789&quot;, 10, 5);
 * BSONDecimal decimal2 = new BSONDecimal(&quot;-123.456789&quot;, 10, 5);
 * System.out.println(decimal1.toString()); // shows 123.45679;
 * System.out.println(decimal2.toString()); // shows -123.45679;
 * </pre>
 * 
 */
public class BSONDecimal implements Serializable {

	private static final long serialVersionUID = 8374882369691286577L;

	final String _value;
	final int _precision;
	final int _scale;

	/**
	 * @fn BSONDecimal(String value, int precision, int scale)
	 * @param value
	 *            the decimal value which is in the format of string, e.g.
	 *            "3.14159265358"
	 * @param precision
	 *            the precision of decimal number
	 * @param scale
	 *            the total count of decimal digits in the fractional part, to
	 *            the right of the decimal point.
	 */
	public BSONDecimal(String value, int precision, int scale) {
		_value = value.trim();
		_precision = precision;
		_scale = scale;
	}

	/**
	 * @fn BSONDecimal(String value)
	 * @param value
	 *            the decimal value which is in the format of string, e.g.
	 *            "3.14159265358"
	 */
	public BSONDecimal(String value) {
		_value = value.trim();
		_precision = -1;
		_scale = -1;
	}

	/**
	 * @fn BSONDecimal(BigDecimal value)
	 * @brief transform a BigDecimal object to a BSONDecimal object.
	 * @param value
	 *            a BigDecimal to be transformed
	 * @note The meaning of "precision" and "scale" defined in BSONDecimal are
	 *       different from that defined by BigDecimal. We deprecate "precision" 
	 *       and "scale" defined in BigDecimal. Actually we build BSONDecimal like
	 *       this:
	 *       <pre>
	 *       BSONDecimal(value.toString().trim()); // value is a BigDecimal object
	 *       </pre>
	 *       <p>
	 *       and set "precision" and "scale" to be -1 in the newly built BSONDecimal
	 *       object.
	 *        </p>
	 * @see toBigDecimal()
	 */
	public BSONDecimal(BigDecimal value) {
		_value = value.toString().trim();
		_precision = -1;
		_scale = -1;
	}

	/**
	 * @fn BigDecimal toBigDecimal()
	 * @brief transform to BigDecimal object
	 * @note The meaning of "precision" and "scale" defined in BSONDecimal are
	 *       different from that defined in BigDecimal.
	 *       <p>
	 *       In BSONDecimal, "precision" represents the total number of digits
	 *       of decimal, and "scale" represents the max count of decimal digits
	 *       in the fractional part. For example, when we define a BSONDecimal
	 *       object which precision is 10 and scale is 6, that means this
	 *       BSONDecimal object can only represent a value which count of digits
	 *       in integer part is not more than 4(10-6) and count of digits in
	 *       fractional part is not more that 6. So, when we specify
	 *       "12345.6789" for this BSONDecimal object, if we insert this object
	 *       into database, we get an error about invalid argument. When we
	 *       specify "1234.1234507" for this BSONDecimal object, if we insert
	 *       this object into database, we finally get "1234.123451". We can get
	 *       only 6 digits after decimal point, and the last digit is rounding
	 *       to 1. when we specify "1.23E10" for this BSONDecial object, we have
	 *       11 digits in the integer part, so when we insert this BSONDecimal
	 *       object into database, we get an error, too.
	 *       </p>
	 *       <p>
	 *       In BigDecimal, the value of the number represented by the
	 *       BigDecimal is (unscaledValue * 10^-scale). "precision" represents
	 *       the count of digits in unscaleValue, and "scale" is the value of
	 *       scale. For example, value "1234.567890" is represented by
	 *       BigDecimal like "1234567890 * 10^-6". So, unscaledValue is
	 *       "1234567890", precision is 10, and scale is 6. Value "1.2345E9" is
	 *       represented by BigDecimal like "12345 * 10^5". So, unscaledValue is
	 *       "12345", precision is 5, and scale is -5.
	 *       </p>
	 *       <p>
	 *       As above, we can see the difference between BSONDecimal and
	 *       BigDecimal. Actually, when we transform BSONDecimal to BigDecimal,
	 *       it's as below:
	 *       </p>
	 * 
	 *       <pre>
	 * new BigDecimal(this.getValue()) // &quot;this&quot; means current BSONDecimal object
	 * </pre>
	 * @return a BigDecimal object
	 */
	public BigDecimal toBigDecimal() {
		return new BigDecimal(this.getValue());
	}

	/**
	 * @fn String getValue()
	 * @brief get the value of decimal
	 * @return the value of decimal
	 */
	public String getValue() {
		return _value;
	}

	/**
	 * @fn int getPrecision()
	 * @brief get the precision of decimal
	 * @return return the <code>precision</code> specified by user or -1 for
	 *         user did not specify it.
	 * @note  When user specify <code>precision</code>, the range of it is [1, 1000]. When user did not specify 
     * <code>precision</code>, it will be set to -1. That means the <code>precision</code> is determined 
     * by database. For decimal, database allow 131072 digits before decimal point and 16383 digits
     * after decimal point at most. 
	 */
	public int getPrecision() {
		return _precision;
	}

	/**
	 * @fn int getScale()
	 * @brief get the scale of the decimal
	 * @return return the <code>scale</code> specified by user or -1 for user
	 *         did not specify it.
	 * @note When user specify <code>scale</code>, the range of it is [0, <code>precision</code>]. 
     * When user did not specify <code>scale</code>, it will be set to -1. 
     * That means the <code>scale</code> is determined 
     * by database. For decimal, database allow 131072 digits before decimal point and 16383 digits
     * after decimal point at most. 
	 */
	public int getScale() {
		return _scale;
	}

    /**
     * @fn boolean equals(Object obj)
     * @brief test current object equals with the other object or not, not care about the precision and scale
     * @return true or false
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == this)
            return true;
        if (obj instanceof BSONDecimal) {
        	BSONDecimal d2 = (BSONDecimal) obj;
        	BigDecimal self = new BigDecimal(getValue());
        	BigDecimal other = new BigDecimal(d2.getValue());
        	return self.compareTo(other) == 0;
        }
        return false;
    }
	
    @Override
	public String toString() {
//		return "{" + "\"$decimal\" : \"" + _value + "\", \"$precision\" : [ " + _precision
//				+ ", " + _scale + " ]" + "}";
		return JSON.serialize(this);
	}
}
