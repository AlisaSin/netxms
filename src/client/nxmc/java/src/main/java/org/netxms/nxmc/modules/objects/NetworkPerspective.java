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
package org.netxms.nxmc.modules.objects;

import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.VPNConnector;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Network" perspective
 */
public class NetworkPerspective extends ObjectsPerspective
{
   public static final I18n i18n = LocalizationHelper.getI18n(NetworkPerspective.class);

   public NetworkPerspective()
   {
      super("Network", i18n.tr("Network"), ResourceManager.getImage("icons/perspective-network.png"), SubtreeType.NETWORK,
            (AbstractObject o) -> {
               if ((o instanceof Interface) || (o instanceof VPNConnector))
                  return false;
               if ((o instanceof Node) && !((Node)o).getPrimaryIP().isValidUnicastAddress())
                  return false;
               return true;
            });
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ObjectsPerspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.priority = 12;
   }
}
