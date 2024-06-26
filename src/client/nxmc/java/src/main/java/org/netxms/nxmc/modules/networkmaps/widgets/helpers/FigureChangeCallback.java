/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapElement;

public interface FigureChangeCallback
{
   /**
    * Method to be called on figure move 
    * 
    * @param element moved element
    */
   void onMove(NetworkMapElement element);
   
   /**
    * Method to be called on link change
    * 
    * @param link changed link
    */
   void onLinkChange(NetworkMapLink link);
}
