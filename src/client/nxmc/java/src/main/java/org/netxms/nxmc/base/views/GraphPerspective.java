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
package org.netxms.nxmc.base.views;

import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.GraphBrowser;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Tools perspective
 */
public class GraphPerspective extends Perspective
{
   static final I18n i18n = LocalizationHelper.getI18n(GraphPerspective.class);

   private NavigationView navigationView;
   private Object previousSelectedElement = null;
   
   public GraphPerspective()
   {
      super("Graphs", i18n.tr("Graphs"), ResourceManager.getImage("icons/perspective-tools.png")); //TODO: change icon
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = true;
      configuration.multiViewNavigationArea = false;
      configuration.multiViewMainArea = false;
      configuration.hasSupplementalArea = false;
      configuration.priority = 250;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      navigationView = new GraphBrowser();
      addNavigationView(navigationView);
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#navigationSelectionChanged(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      Object currentElement = selection.getFirstElement();

      if (previousSelectedElement == currentElement)
         return; //do nothing for reselection

      if (currentElement != null && currentElement instanceof GraphDefinition)
      {
         HistoricalGraphView view = new HistoricalGraphView();
         view.setShowDeleteAction(true);
         setMainView(view);
         view.initPredefinedGraph((GraphDefinition)currentElement);
      }
      else
      {
         setMainView(null);
      }
      previousSelectedElement = currentElement;
   }
}
