/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.modules.nxsl.views;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
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
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.dialogs.CreateScriptDialog;
import org.netxms.nxmc.modules.nxsl.views.helpers.ScriptComparator;
import org.netxms.nxmc.modules.nxsl.views.helpers.ScriptFilter;
import org.netxms.nxmc.modules.nxsl.views.helpers.ScriptLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Script library view
 */
public class ScriptLibraryView extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ScriptLibraryView.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;

   private static final String TABLE_CONFIG_PREFIX = "ScriptLibrary";

   private NXCSession session;
   private SortableTableViewer viewer;
   private ScriptFilter filter;
   private FilterText filterText;
   private Composite content;
   private RefreshAction actionRefresh;
   private Action actionNew;
   private Action actionEdit;
   private Action actionRename;
   private Action actionDelete;
   private Action actionCopyName;
   private Action actionShowFilter;

   /**
    * Create script library view
    */
   public ScriptLibraryView()
   {
      super(i18n.tr("Script Library"), ResourceManager.getImageDescriptor("icons/config-views/script_library.png"));
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());

      // Create filter area
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });

      final String[] names = { i18n.tr("ID"), i18n.tr("Name") };
      final int[] widths = { 90, 500 };
      viewer = new SortableTableViewer(content, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      WidgetHelper.restoreTableViewerSettings(viewer, PreferenceStore.getInstance(), TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ScriptLabelProvider());
      viewer.setComparator(new ScriptComparator());
      filter = new ScriptFilter();
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionRename.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
               actionCopyName.setEnabled(selection.size() == 1);
            }
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, PreferenceStore.getInstance(), TABLE_CONFIG_PREFIX);
         }
      });

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

      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

      createActions();
      createPopupMenu();

      // Set initial focus to filter input line
      if (actionShowFilter.isChecked())
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      refreshScriptList();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            refreshScriptList();
         }
      };

      actionNew = new Action(i18n.tr("Create &new script..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewScript();
         }
      };

      actionEdit = new Action(i18n.tr("&Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editScript();
         }
      };
      actionEdit.setEnabled(false);

      actionRename = new Action(i18n.tr("&Rename...")) {
         @Override
         public void run()
         {
            renameScript();
         }
      };
      actionRename.setEnabled(false);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteScript();
         }
      };
      actionDelete.setEnabled(false);

      actionCopyName = new Action(i18n.tr("&Copy name to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyNameToClipboard();
         }
      };

      actionShowFilter = new Action(i18n.tr("Show &filter"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(PreferenceStore.getInstance().getAsBoolean("ScriptLibrary.showFilter", true));
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
      Menu menu = menuMgr.createContextMenu(viewer.getTable());
      viewer.getTable().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      mgr.add(actionNew);
      mgr.add(actionEdit);
      mgr.add(actionRename);
      mgr.add(actionDelete);
      mgr.add(actionCopyName);
   }

   /**
    * Reload script list from server
    */
   private void refreshScriptList()
   {
      new Job(i18n.tr("Loading library script list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<Script> library = session.getScriptLibrary();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(library.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load list of library scripts");
         }
      }.start();
   }

   /**
    * Create new script
    */
   private void createNewScript()
   {
      final CreateScriptDialog dlg = new CreateScriptDialog(getWindow().getShell(), null);
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Creating new script"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final long id = session.modifyScript(0, dlg.getName(), ""); //$NON-NLS-1$
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     Object[] input = (Object[])viewer.getInput();
                     List<Script> list = new ArrayList<Script>(input.length);
                     for(Object o : input)
                        list.add((Script)o);
                     final Script script = new Script(id, dlg.getName(), ""); //$NON-NLS-1$
                     list.add(script);
                     viewer.setInput(list.toArray());
                     viewer.setSelection(new StructuredSelection(script));
                     actionEdit.run();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot create new script in library");
            }
         }.start();
      }
   }

   /**
    * Edit script
    */
   private void editScript()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      Script script = (Script)selection.getFirstElement();
      // TODO: open editor
   }

   /**
    * Edit script
    */
   private void renameScript()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      final Script script = (Script)selection.getFirstElement();
      final CreateScriptDialog dlg = new CreateScriptDialog(getWindow().getShell(), script.getName());
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Rename library script"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.renameScript(script.getId(), dlg.getName());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     Object[] input = (Object[])viewer.getInput();
                     List<Script> list = new ArrayList<Script>(input.length);
                     for(Object o : input)
                     {
                        if (((Script)o).getId() != script.getId())
                           list.add((Script)o);
                     }
                     final Script newScript = new Script(script.getId(), dlg.getName(), script.getSource());
                     list.add(newScript);
                     viewer.setInput(list.toArray());
                     viewer.setSelection(new StructuredSelection(newScript));
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot rename library script");
            }
         }.start();
      }
   }

   /**
    * Delete selected script(s)
    */
   @SuppressWarnings("rawtypes")
   private void deleteScript()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Do you really want to delete selected scripts?")))
         return;

      new Job(i18n.tr("Delete scripts from library"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            Iterator it = selection.iterator();
            while(it.hasNext())
            {
               Script script = (Script)it.next();
               session.deleteScript(script.getId());
            }
         }

         @Override
         protected void jobFinalize()
         {
            refreshScriptList();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete script from library");
         }
      }.start();
   }

   /**
    * Copy script name to clipboard
    */
   private void copyNameToClipboard()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      WidgetHelper.copyToClipboard(((Script)selection.getFirstElement()).getName());
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterText.setVisible(enable);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      PreferenceStore.getInstance().set("ScriptLibrary.showFilter", enable);
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
}
