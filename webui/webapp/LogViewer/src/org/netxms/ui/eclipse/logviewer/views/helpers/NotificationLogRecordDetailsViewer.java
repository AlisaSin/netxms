/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Solutions
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
package org.netxms.ui.eclipse.logviewer.views.helpers;

import org.eclipse.ui.IViewPart;
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogRecordDetails;
import org.netxms.ui.eclipse.logviewer.LogRecordDetailsViewer;
import org.netxms.ui.eclipse.logviewer.dialogs.NotificationLogRecordDetailsDialog;

/**
 * Log record details viewer for Windows event log
 */
public class NotificationLogRecordDetailsViewer implements LogRecordDetailsViewer
{
   /**
    * @see org.netxms.ui.eclipse.logviewer.LogRecordDetailsViewer#showRecordDetails(org.netxms.client.log.LogRecordDetails,
    *      org.netxms.client.TableRow, org.netxms.client.log.Log, org.eclipse.ui.IViewPart)
    */
   @Override
   public void showRecordDetails(LogRecordDetails details, TableRow record, Log logHandle, IViewPart viewPart)
   {
      NotificationLogRecordDetailsDialog dlg = new NotificationLogRecordDetailsDialog(viewPart.getSite().getShell(), record, logHandle);
      dlg.open();
   }
}
