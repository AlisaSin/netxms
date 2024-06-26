/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.actions;

import org.netxms.ui.eclipse.actions.OpenView;
import org.netxms.ui.eclipse.agentmanager.views.UserAgentNotificationView;

/**
 * Open user support application notification view
 */
public class OpenUserAgentNotificationView extends OpenView
{
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.actions.OpenView#getViewId()
    */
   @Override
   protected String getViewId()
   {
      return UserAgentNotificationView.ID;
   }
}
