/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.alarms;

import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.services.LoginListener;

/**
 * Login listener for alarms module
 */
public class AlarmsModuleLoginListener implements LoginListener
{
   /**
    * @see org.netxms.nxmc.services.LoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
    */
   @Override
   public void afterLogin(NXCSession session, Display display)
   {
      AlarmNotifier.init(session, display);
   }
}
