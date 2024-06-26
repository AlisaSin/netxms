/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.events.widgets.EventTemplateList;

/**
 * Event template comparator
 */
public class EventTemplateComparator extends ViewerComparator
{
   boolean isDialog;
   
   /**
    * Create event template comparator.
    *
    * @param isDialog true if owning widget is part of dialog
    */
	public EventTemplateComparator(boolean isDialog)
   {
      this.isDialog = isDialog;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case EventTemplateList.COLUMN_CODE:
            result = (int)(((EventTemplate)e1).getCode() - ((EventTemplate)e2).getCode());
				break;
			case EventTemplateList.COLUMN_NAME:
            result = ((EventTemplate)e1).getName().compareToIgnoreCase(((EventTemplate)e2).getName());
				break;
			case EventTemplateList.COLUMN_SEVERITY_OR_TAGS:
			   if(isDialog)
			      result = ((EventTemplate)e1).getTagList().compareToIgnoreCase(((EventTemplate)e2).getTagList());
			   else
			      result = ((EventTemplate)e1).getSeverity().compareTo(((EventTemplate)e2).getSeverity());
				break;
			case EventTemplateList.COLUMN_FLAGS:
            result = ((EventTemplate)e1).getFlags() - ((EventTemplate)e2).getFlags();
				break;
			case EventTemplateList.COLUMN_MESSAGE:
            result = ((EventTemplate)e1).getMessage().compareToIgnoreCase(((EventTemplate)e2).getMessage());
				break;
         case EventTemplateList.COLUMN_TAGS:
            result = ((EventTemplate)e1).getTagList().compareToIgnoreCase(((EventTemplate)e2).getTagList());
            break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
