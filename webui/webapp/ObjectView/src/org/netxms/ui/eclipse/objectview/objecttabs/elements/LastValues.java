/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.OverviewDciLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.tools.VisibilityValidator;

/**
 * DCI last values
 */
public class LastValues extends OverviewPageElement
{
   private TableViewer viewer;
   private ViewRefreshController refreshController;
   
   /**
    * @param parent
    * @param anchor
    * @param objectTab
    */
   public LastValues(Composite parent, OverviewPageElement anchor, ObjectTab objectTab)
   {
      super(parent, anchor, objectTab);
      refreshController = new ViewRefreshController(getObjectTab().getViewPart(), -1, new Runnable() {
         @Override
         public void run()
         {
            refresh();
         }
      }, new VisibilityValidator() {
         @Override
         public boolean isVisible()
         {
            return getObjectTab().isActive();
         }
      });      
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return Messages.get().LastValues_Title;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      if (!(object instanceof DataCollectionTarget))
         return false;
      return !((DataCollectionTarget)object).getOverviewDciData().isEmpty();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      viewer = new TableViewer(parent, SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.H_SCROLL | SWT.MULTI);
      setupTable();
      
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new OverviewDciLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((DciValue)e1).getDescription().compareToIgnoreCase(((DciValue)e2).getDescription());
         }
      });
      
      viewer.setInput(((DataCollectionTarget)getObject()).getOverviewDciData().toArray());
      adjustCollumns();
      
      createPopupMenu();
      
      refreshController.setInterval(30);
      refresh();

      return viewer.getTable();
   }
   
   /**
    * Adjust column size 
    */
   private void adjustCollumns()
   {
      for(TableColumn cl : viewer.getTable().getColumns())
      {
         cl.pack();
         cl.setWidth(cl.getWidth() + 10); // compensate for pack issues on Linux         
      }
   }
   
   /**
    * Setup table widget
    */
   private void setupTable()
   {
      TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText(Messages.get().LastValues_Description);
      tc.setWidth(300);

      tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText(Messages.get().LastValues_Value);
      tc.setWidth(100);

      tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText("Time");
      tc.setWidth(100);

      viewer.getTable().setHeaderVisible(false);
      viewer.getTable().setLinesVisible(false);
   }
   
   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension.
      getObjectTab().getViewPart().getSite().registerContextMenu(menuMgr, viewer);
   }
   
   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_SECONDARY));
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      viewer.setInput(((DataCollectionTarget)getObject()).getOverviewDciData().toArray());
      adjustCollumns();
   }
   
   /**
    * Refresh view
    */
   private void refresh()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      final long nodeId = getObject().getObjectId();
      ConsoleJob job = new ConsoleJob(Messages.get().LastValues_JobName, getObjectTab().getViewPart(), Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final DciValue[] list = session.getDataCollectionSummary(nodeId, false, true, false);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getControl().isDisposed() || (getObject().getObjectId() != nodeId))
                        return;
                     viewer.setInput(list);
                     adjustCollumns();
                  }
               });
            }
            catch(Exception e)
            {
               Activator.log("Exception in last values overview element", e); //$NON-NLS-1$
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().LastValues_JobErrorMsg;
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }
}
