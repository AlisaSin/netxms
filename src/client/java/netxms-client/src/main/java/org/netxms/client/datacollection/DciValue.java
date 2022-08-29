/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.Severity;

/**
 * DCI value
 */
public abstract class DciValue
{
   final public static int MULTIPLIERS_DEFAULT = 0;
   final public static int MULTIPLIERS_YES = 1;
   final public static int MULTIPLIERS_NO = 2;

	protected long id;					// DCI id
	protected long nodeId;				// related node object id
   protected long templateDciId; // related template DCI ID
	protected String name;				// name
	protected String description;	// description
	protected String value;			// value
   protected DataOrigin source;  // data source (agent, SNMP, etc.)
	protected DataType dataType;
	protected int status;				// status (active, disabled, etc.)
   protected int errorCount;
   protected int dcObjectType; // Data collection object type (item, table, etc.)
   protected Date timestamp;
   protected Threshold activeThreshold;
   protected int flags;
   protected MeasurementUnit measurementUnit;

   /**
    * Factory method to create correct DciValue subclass from NXCP message.
    * 
    * @param msg NXCP message
    * @param base Base variable ID for value object
    * @return DciValue object
    */
	public static DciValue createFromMessage(NXCPMessage msg, long base)
	{
		int type = msg.getFieldAsInt32(base + 10);
		switch(type)
		{
			case DataCollectionObject.DCO_TYPE_ITEM:
				return new SimpleDciValue(msg, base);
			case DataCollectionObject.DCO_TYPE_TABLE:
				return new TableDciValue(msg, base);
			default:
				return null;
		}
	}

	/**
	 * Simple constructor for DciValue
	 */
	protected DciValue()
   {
   }
	
	/**
	 * Constructor for creating DciValue from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param base Base field ID for value object
	 */
	protected DciValue(NXCPMessage msg, long base)
	{
		long fieldId = base;

      nodeId = msg.getFieldAsInt64(fieldId++);
		id = msg.getFieldAsInt64(fieldId++);
		name = msg.getFieldAsString(fieldId++);
      flags = msg.getFieldAsInt32(fieldId++);
		description = msg.getFieldAsString(fieldId++);
      source = DataOrigin.getByValue(msg.getFieldAsInt32(fieldId++));
		dataType = DataType.getByValue(msg.getFieldAsInt32(fieldId++));
		value = msg.getFieldAsString(fieldId++);
		timestamp = msg.getFieldAsDate(fieldId++);
		status = msg.getFieldAsInt32(fieldId++);
		dcObjectType = msg.getFieldAsInt32(fieldId++);
		errorCount = msg.getFieldAsInt32(fieldId++);
		templateDciId = msg.getFieldAsInt64(fieldId++);
      measurementUnit = new MeasurementUnit(msg, fieldId);
      fieldId += 2;
		if (msg.getFieldAsBoolean(fieldId++))
			activeThreshold = new Threshold(msg, fieldId);
		else
			activeThreshold = null;
				
	}
	
   /**
	 * Returns formated DCI value or string with format error and correct type of DCI value;
	 * 
	 * @param formatString the string into which will be placed DCI value 
	 * @param formatter date/time formatter
	 * @return The format
	 */
	public String format(String formatString, TimeFormatter formatter)
	{     
      return new DataFormatter(formatString, dataType, measurementUnit).format(value, formatter);
	}

   /**
    * Returns formated DCI value or string with format error and correct type of DCI value;
    * 
    * @param useMultipliers the string into which will be placed DCI value 
    * @return The format
    */
   public String getFormattedValue(boolean useMultipliers, TimeFormatter formatter)
   {     
      int selection = getMultipliersSelection();
      String format = ((selection == DciValue.MULTIPLIERS_DEFAULT) && useMultipliers) || 
            (selection == DciValue.MULTIPLIERS_YES) ? "%{m,u}s" : "%{u}s" ;
      return new DataFormatter(format, dataType, measurementUnit).format(value, formatter);
   }

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name.isEmpty() ? "[" + getId() + "]" : name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @return the source
	 */
   public DataOrigin getSource()
	{
		return source;
	}

	/**
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the activeThreshold
	 */
	public Threshold getActiveThreshold()
	{
		return activeThreshold;
	}

	/**
	 * @return the dcObjectType
	 */
	public int getDcObjectType()
	{
		return dcObjectType;
	}

	/**
	 * @return the errorCount
	 */
	public int getErrorCount()
	{
		return errorCount;
	}

	/**
	 * @return the templateDciId
	 */
	public final long getTemplateDciId()
	{
		return templateDciId;
	}

   /**
    * Get severity of active threshold
    * 
    * @return severity of active threshold or NORMAL if there are no active thresholds
    */
   public Severity getThresholdSeverity()
   {
      return (activeThreshold != null) ? activeThreshold.getCurrentSeverity() : Severity.NORMAL;
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }
   
   /**
    * Get multipliers selection.
    * 
    * @return multiplier usage mode (DEFAULT, YES, or NO)
    */
   public int getMultipliersSelection()
   {
      return (int)((flags & DataCollectionItem.DCF_MULTIPLIERS_MASK) >> 16);
   }
   
   /**
    * @return the mostCriticalSeverity
    */
   public Severity getMostCriticalSeverity()
   {
      return getThresholdSeverity();
   }
   
   /**
    * @return the unitName
    */
   public MeasurementUnit getMeasurementUnit()
   {
      return measurementUnit;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DciValue [id=" + id + ", nodeId=" + nodeId + ", templateDciId=" + templateDciId + ", name=" + name + ", description=" + description + ", value=" + value + ", source=" + source +
            ", dataType=" + dataType + ", status=" + status + ", errorCount=" + errorCount + ", dcObjectType=" + dcObjectType + ", timestamp=" + timestamp + ", activeThreshold=" + activeThreshold +
            ", flags=" + flags + ", measurementUnit=" + measurementUnit + "]";
   }
}
