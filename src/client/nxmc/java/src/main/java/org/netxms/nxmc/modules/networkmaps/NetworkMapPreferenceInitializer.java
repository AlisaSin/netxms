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
package org.netxms.nxmc.modules.networkmaps;

import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.services.PreferenceInitializer;

/**
 * Preference initializer for network maps
 */
public class NetworkMapPreferenceInitializer implements PreferenceInitializer
{
   /**
    * @see org.netxms.nxmc.services.PreferenceInitializer#initializeDefaultPreferences(org.netxms.nxmc.PreferenceStore)
    */
   @Override
   public void initializeDefaultPreferences(PreferenceStore ps)
   {
      ps.setDefault("NetMap.DefaultLinkWidth", 5);
   }
}
