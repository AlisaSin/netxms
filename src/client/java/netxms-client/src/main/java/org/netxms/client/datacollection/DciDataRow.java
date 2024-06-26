/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.Date;

/**
 * Single row in DCI data
 */
public class DciDataRow
{
	private Date timestamp;
	private Object value;
   private String rawValue;

	public DciDataRow(Date timestamp, Object value)
	{
		super();
		this.timestamp = timestamp;
		this.value = value;
		this.rawValue = null;
	}

	/**
	 * Set raw value
	 * 
	 * @param rawValue new raw value
	 */
   public void setRawValue(String rawValue)
	{
	   this.rawValue = rawValue;
	}
	
	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the value
	 */
	public Object getValue()
	{
		return value;
	}
	
	/**
	 * @return the value
	 */
	public String getValueAsString()
	{
		return (value != null) ? value.toString() : "";
	}

	/**
	 * @return the value
	 */
	public long getValueAsLong()
	{
		if (value instanceof Long)
			return ((Long)value).longValue();

		if (value instanceof Double)
			return ((Double)value).longValue();
		
      if (value instanceof String)
      {
         try
         {
            return Long.parseLong((String)value);
         }
         catch(NumberFormatException e)
         {
            return 0;
         }
      }
      
		return 0;
	}

	/**
	 * @return the value
	 */
	public double getValueAsDouble()
	{
		if (value instanceof Long)
			return ((Long)value).doubleValue();

		if (value instanceof Double)
			return ((Double)value).doubleValue();
		
      if (value instanceof String)
      {
         try
         {
            return Double.parseDouble((String)value);
         }
         catch(NumberFormatException e)
         {
            return 0;
         }
      }
		
		return 0;
	}

   /**
    * @return raw value
    */
   public String getRawValue()
   {
      return rawValue;
   }
	
   /**
    * Invert value
    */
   public void invert()
   {
      if (value instanceof Long)
         value = -((Long)value);
      else if (value instanceof Double)
         value = -((Double)value);
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DciDataRow [timestamp=" + timestamp + ", value=" + value + ", rawValue=" + rawValue + "]";
   }
}
