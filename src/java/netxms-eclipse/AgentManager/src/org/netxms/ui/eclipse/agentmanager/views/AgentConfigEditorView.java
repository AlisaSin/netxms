/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.text.IFindReplaceTarget;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.texteditor.FindReplaceAction;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveConfigDialog;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.NXFindAndReplaceAction;
import org.netxms.ui.eclipse.widgets.AgentConfigEditor;

/**
 * Agent's master config editor
 */
public class AgentConfigEditorView extends ViewPart implements ISaveablePart2
{
	public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.AgentConfigEditorView"; //$NON-NLS-1$

	private NXCSession session;
	private long nodeId;
	private AgentConfigEditor editor;
	private boolean modified = false;
   private boolean dirty = false;
	private boolean saveAndApply = false;
	private String saveData;

	private RefreshAction actionRefresh;
	private Action actionSave;
	private FindReplaceAction actionFindReplace;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		nodeId = Long.parseLong(site.getSecondaryId());
		
		AbstractObject object = session.findObjectById(nodeId);
		setPartName(Messages.get().AgentConfigEditorView_PartName + ((object != null) ? object.getObjectName() : Long.toString(nodeId)));
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
		parent.setLayout(new FillLayout());
		
		editor = new AgentConfigEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL);
		editor.getTextWidget().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				if (!dirty)
				{
					modified = true;
					dirty = true;
					firePropertyChange(PROP_DIRTY);
					actionSave.setEnabled(true);
					actionFindReplace.update();
				}
			}
		});
		
      activateContext();
		createActions();
		contributeToActionBars();
      actionSave.setEnabled(false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		editor.setFocus();
	}

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.agentmanager.context.ConfigEditor"); //$NON-NLS-1$
      }
   }
	
	/**
	 * Set configuration file content
	 * 
	 * @param config
	 */
	public void setConfig(final String config)
	{
		editor.setText(config);
		actionFindReplace.update();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionFindReplace = NXFindAndReplaceAction.getFindReplaceAction(this);
		
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
			}
		};
		
		actionSave = new Action() {
			@Override
			public void run()
			{
			   doSave(null);
			}
		};
		actionSave.setText(Messages.get().AgentConfigEditorView_Save);
		actionSave.setImageDescriptor(SharedIcons.SAVE);
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionFindReplace);
		manager.add(actionSave);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionSave);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		try
		{
	      saveData = editor.getText();
			session.writeAgentConfigurationFile(nodeId, saveData, saveAndApply);
	      actionSave.setEnabled(false);
	      saveAndApply = false;
	      dirty = false;
	      modified = false;
	      firePropertyChange(PROP_DIRTY);
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getViewSite().getShell(), Messages.get().AgentConfigEditorView_Error, Messages.get().AgentConfigEditorView_SaveError + e.getMessage());
		}
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
	@Override
	public void doSaveAs()
	{
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
	@Override
	public boolean isDirty()
	{
		return dirty || modified;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return modified;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart2#promptToSaveOnClose()
    */
	@Override
	public int promptToSaveOnClose()
	{
		SaveConfigDialog dlg = new SaveConfigDialog(getSite().getShell());
		int rc = dlg.open();
		saveAndApply = (rc == SaveConfigDialog.SAVE_AND_APPLY_ID);
		return (rc == IDialogConstants.CANCEL_ID) ? CANCEL : ((rc == SaveConfigDialog.DISCARD_ID) ? NO : YES);
	}

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#getAdapter(java.lang.Class)
    */
   @SuppressWarnings("unchecked")
   @Override
   public <T> T getAdapter(Class<T> adapter)
   {
      if (adapter == IFindReplaceTarget.class)
         return (T)editor.getFindReplaceTarget();
      return super.getAdapter(adapter);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      dirty = false;
      modified = false;
      firePropertyChange(PROP_DIRTY);

      super.dispose();
   }
}
