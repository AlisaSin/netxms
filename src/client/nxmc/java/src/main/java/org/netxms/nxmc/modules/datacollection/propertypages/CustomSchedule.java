/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.HashSet;
import java.util.Iterator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.nxmc.base.helpers.StringComparator;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.dialogs.EditScheduleDialog;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.CustomScheduleLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Custom Schedule" property page
 */
public class CustomSchedule extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(CustomSchedule.class);
   
	private DataCollectionObject dci;
	private HashSet<String> schedules;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	
	/**
	 * 
	 * @param editor
	 */
   public CustomSchedule(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(CustomSchedule.class).tr("Custom Schedule"), editor);
   }

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObject();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { i18n.tr("Schedule"), i18n.tr("Description") };
      final int[] columnWidths = { 300, 300 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new StringComparator());
      viewer.setLabelProvider(new CustomScheduleLabelProvider());
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = event.getStructuredSelection();
				if (selection != null)
				{
					editButton.setEnabled(selection.size() == 1);
					deleteButton.setEnabled(selection.size() > 0);
				}
			}
		});
      
      schedules = new HashSet<String>();
      schedules.addAll(dci.getSchedules());
      viewer.setInput(schedules.toArray());
      
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      viewer.getControl().setLayoutData(gridData);
      
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.marginBottom = 0;
      buttonLayout.marginTop = 0;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonLayout.fill = true;
      buttonLayout.pack = false;
      buttons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gridData);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addSchedule();
			}
      });
		
      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editSchedule();
			}
      });
		
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteSchedules();
			}
      });
		
      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editButton.notifyListeners(SWT.Selection, new Event());
			}
      });

      return dialogArea;
	}
	
	/**
	 * Add new schedule to list
	 */
	private void addSchedule()
	{
		EditScheduleDialog dlg = new EditScheduleDialog(getShell(), ""); //$NON-NLS-1$
		if (dlg.open() == Window.OK)
		{
			schedules.add(dlg.getSchedule());
			viewer.setInput(schedules.toArray());
			viewer.setSelection(new StructuredSelection(dlg.getSchedule()));
		}
	}
	
	/**
	 * Edit currently selected schedule
	 */
	private void editSchedule()
	{
		IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() != 1)
			return;
		
		final String oldValue = new String((String)selection.getFirstElement());
		EditScheduleDialog dlg = new EditScheduleDialog(getShell(), (String)selection.getFirstElement());
		if (dlg.open() == Window.OK)
		{
			schedules.remove(oldValue);
			schedules.add(dlg.getSchedule());
			viewer.setInput(schedules.toArray());
			viewer.setSelection(new StructuredSelection(dlg.getSchedule()));
		}
	}
	
	/**
	 * Delete selected schedules
	 */
	private void deleteSchedules()
	{
		IStructuredSelection selection = viewer.getStructuredSelection();
		Iterator<?> it = selection.iterator();
		while(it.hasNext())
		{
			schedules.remove(it.next());
		}
		viewer.setInput(schedules.toArray());
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		dci.setSchedules(schedules);
		editor.modify();
		return true;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		schedules.clear();
		viewer.setInput(schedules.toArray());
	}
}
