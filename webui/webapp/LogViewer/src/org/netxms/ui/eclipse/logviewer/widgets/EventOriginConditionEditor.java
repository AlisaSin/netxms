/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.logviewer.views.helpers.LogLabelProvider;

/**
 * Condition editor for event origin columns
 */
public class EventOriginConditionEditor extends ConditionEditor
{
	private static final String[] OPERATIONS = { Messages.get().AlarmStateConditionEditor_Is, Messages.get().AlarmStateConditionEditor_IsNot };
	
	private Combo state;
	
	/**
	 * @param parent
	 */
   public EventOriginConditionEditor(Composite parent)
	{
      super(parent);
	}

   /**
    * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
    */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

   /**
    * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.netxms.client.log.ColumnFilter)
    */
	@Override
   protected void createContent(ColumnFilter initialFilter)
	{
		state = new Combo(this, SWT.READ_ONLY | SWT.BORDER);
		for(String origin : LogLabelProvider.getEmptyInstance().EVENT_ORIGIN_TEXTS)
			state.add(origin);
		state.select(0);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
		state.setLayoutData(gd);

      if ((initialFilter != null) && (initialFilter.getType() == ColumnFilterType.EQUALS))
      {
         setSelectedOperation(initialFilter.isNegated() ? 1 : 0);
         state.select((int)initialFilter.getNumericValue());
      }
	}

   /**
    * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
    */
	@Override
	public ColumnFilter createFilter()
	{
		ColumnFilter filter = new ColumnFilter(ColumnFilterType.EQUALS, state.getSelectionIndex());
		filter.setNegated(getSelectedOperation() == 1);
		return filter;
	}
}
