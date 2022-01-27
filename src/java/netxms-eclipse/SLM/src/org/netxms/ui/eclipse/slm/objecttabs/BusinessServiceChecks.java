/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.ui.eclipse.slm.objecttabs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BaseBusinessService;
import org.netxms.client.objects.interfaces.NodeItemPair;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.slm.Activator;
import org.netxms.ui.eclipse.slm.dialogs.EditBusinessServiceCheckDlg;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceCheckFilter;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceCheckLabelProvider;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceComparator;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Checks for business services
 */
public class BusinessServiceChecks extends ObjectTab
{
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_TYPE = 2;
   public static final int COLUMN_OBJECT = 3;
   public static final int COLUMN_DCI = 4;
   public static final int COLUMN_STATUS = 5;
   public static final int COLUMN_FAIL_REASON = 6;

   private static final String CONFIG_PREFIX = "BusinessServiceChecks";

   private NXCSession session;
   private SessionListener sessionListener;
   private Composite content;
   private SortableTableViewer viewer;
   private FilterText filterText;
   private boolean filterEnabled;
   private BusinessServiceCheckLabelProvider labelProvider;
   private BusinessServiceCheckFilter filter;
   private Action actionEdit;
   private Action actionCreate;
   private Action actionDelete;
   private Map<Long, BusinessServiceCheck> checks;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   { 
      session = ConsoleSharedData.getSession();

      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());

      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
            Command command = service.getCommand("org.netxms.ui.eclipse.slm.commands.show_checks_filter"); //$NON-NLS-1$
            State state = command.getState("org.netxms.ui.eclipse.slm.commands.show_checks_filter.state"); //$NON-NLS-1$
            state.setValue(false);
            service.refreshElements(command.getId(), null);
         }
      });

      // Setup table columns
      final String[] names = { "ID", "Description", "Type", "Object", "DCI", "Status", "Reason" };
      final int[] widths = { 70, 200, 100, 200, 200, 70, 300 };
      viewer = new SortableTableViewer(content, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
      labelProvider = new BusinessServiceCheckLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new BusinessServiceComparator(labelProvider));
      filter = new BusinessServiceCheckFilter(labelProvider);
      viewer.addFilter(filter);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = event.getStructuredSelection();
            actionEdit.setEnabled(selection.size() == 1);
            actionDelete.setEnabled(selection.size() > 0);
         }
      });

      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      filterEnabled = settings.getBoolean(CONFIG_PREFIX + ".EnableFilter");
      WidgetHelper.restoreColumnSettings(viewer.getTable(), settings, CONFIG_PREFIX + ".TableSettings");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), settings, CONFIG_PREFIX + ".TableSettings");
            settings.put(CONFIG_PREFIX + ".EnableFilter", filterEnabled);
         }
      });

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      createActions();
      createPopupMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.BUSINESS_SERVICE_CHECK_MODIFY:
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        if (checks == null)
                           return;
                        checks.put(n.getSubCode(), (BusinessServiceCheck)n.getObject());
                        List<BusinessServiceCheck> newCheckArray = Arrays.asList((BusinessServiceCheck)n.getObject());
                        updateDciLabels(newCheckArray);
                        syncMissingObjects(newCheckArray);
                        viewer.refresh();
                     }
                  });
                  break;
               case SessionNotification.BUSINESS_SERVICE_CHECK_DELETE:
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        if (checks == null)
                           return;
                        checks.remove(n.getSubCode());
                        viewer.refresh();
                     }
                  });
                  break;
            }
         }
      };
      session.addListener(sessionListener);

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTable().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreate = new Action("&New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createCheck();
         }
      };

      actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editCheck();
         }
      };

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteCheck();
         }
      };
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
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(actionEdit);
      manager.add(actionDelete);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return object instanceof BaseBusinessService;
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      refresh();
      ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
      Command command = service.getCommand("org.netxms.ui.eclipse.slm.commands.show_checks_filter"); //$NON-NLS-1$
      State state = command.getState("org.netxms.ui.eclipse.slm.commands.show_checks_filter.state"); //$NON-NLS-1$
      state.setValue(filterEnabled);
      service.refreshElements(command.getId(), null);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(AbstractObject object)
   {
      viewer.setInput(new Object[0]);
      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      final AbstractObject object = getObject();
      if (object == null)
         return;

      new ConsoleJob("Get business service checks", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            Map<Long, BusinessServiceCheck> checks = session.getBusinessServiceChecks(object.getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  BusinessServiceChecks.this.checks = checks;
                  viewer.setInput(checks.values());
                  syncMissingObjects(checks.values());
                  updateDciLabels(checks.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get checks for business service %s", object.getObjectName());
         }
      }.start();
   }

   /**
    * Update DCI labels in check list
    * 
    * @param checks
    */
   public void updateDciLabels(Collection<BusinessServiceCheck> checks)
   {
      ConsoleJob job = new ConsoleJob("Resolve DCI names", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Map<Long, String> names = session.dciIdsToNames(checks);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  labelProvider.updateDciNames(names);
                  viewer.refresh(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot resolve DCI names";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Sync missing objects
    * 
    * @param checks
    */
   public void syncMissingObjects(Collection<BusinessServiceCheck> checks)
   {
      ConsoleJob job = new ConsoleJob("Sync objects", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {            
            List<Long> relatedOpbjects = new ArrayList<Long>();
            for (NodeItemPair pair : checks)
            {
               if (pair.getNodeId() != 0)
               relatedOpbjects.add(pair.getNodeId());
            }
            session.syncMissingObjects(relatedOpbjects, true, NXCSession.OBJECT_SYNC_WAIT);
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     viewer.refresh(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Failed to sync objects";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Delete selected check(s)
    */
   protected void deleteCheck()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getViewPart().getSite().getShell(), "Confirm Delete", "Do you really want to delete selected check?"))
         return;

      final Object[] objects = selection.toArray();
      new ConsoleJob("Delete business service check", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteBusinessServiceCheck(getObject().getObjectId(), ((BusinessServiceCheck)objects[i]).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete business service check";
         }
      }.start();
   }

   /**
    * Edit selected check
    */
   protected void editCheck()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final BusinessServiceCheck check = new BusinessServiceCheck((BusinessServiceCheck)selection.getFirstElement());
      final EditBusinessServiceCheckDlg dlg = new EditBusinessServiceCheckDlg(getViewPart().getSite().getShell(), check, false);
      if (dlg.open() == Window.OK)
      {
         new ConsoleJob("Update business service check", getViewPart(), Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.modifyBusinessServiceCheck(getObject().getObjectId(), check);
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot update business service check";
            }
         }.start();
      }
   }

   /**
    * Create new check
    */
   private void createCheck()
   {
      final BusinessServiceCheck check = new BusinessServiceCheck();
      final EditBusinessServiceCheckDlg dlg = new EditBusinessServiceCheckDlg(getViewPart().getSite().getShell(), check, false);
      if (dlg.open() == Window.OK)
      {
         new ConsoleJob("Create business service check", getViewPart(), Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.modifyBusinessServiceCheck(getObject().getObjectId(), check);
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot create business service check";
            }
         }.start();
      }
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterText.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filter.setFilterString(""); //$NON-NLS-1$
         viewer.refresh(false);
      }
   }
}
