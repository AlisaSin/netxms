/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Zone object creation dialog
 *
 */
public class CreateZoneDialog extends Dialog
{
	private LabeledText nameField;
   private LabeledText aliasField;
	private LabeledText uinField;
	
	private String name;
   private String alias;
	private int zoneUIN;
	
	/**
	 * @param parentShell
	 */
	public CreateZoneDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().CreateZoneDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		nameField = new LabeledText(dialogArea, SWT.NONE);
		nameField.setLabel(Messages.get().CreateZoneDialog_Name);
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		nameField.setLayoutData(gd);
		
      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(Messages.get().CreateZoneDialog_Alias);
      aliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      aliasField.setLayoutData(gd);

		uinField = new LabeledText(dialogArea, SWT.NONE);
		uinField.setLabel(Messages.get().CreateZoneDialog_ZoneId);
		uinField.getTextControl().setTextLimit(10);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		uinField.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
	   String zoneIdText = uinField.getText().trim();
	   if (!zoneIdText.isEmpty())
	   {
   		try
   		{
   			zoneUIN = Integer.parseInt(uinField.getText());
   		}
   		catch(NumberFormatException e)
   		{
   			MessageDialogHelper.openWarning(getShell(), Messages.get().CreateZoneDialog_Warning, Messages.get().CreateZoneDialog_WarningInvalidZoneId);
   			return;
   		}
   		if (zoneUIN <= 0)
   		{
   			MessageDialogHelper.openWarning(getShell(), Messages.get().CreateZoneDialog_Warning, Messages.get().CreateZoneDialog_WarningInvalidZoneId);
   			return;
   		}
	   }
	   else
	   {
	      zoneUIN = 0;
	   }
		
		name = nameField.getText().trim();
      alias = aliasField.getText().trim();
		if (name.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().CreateZoneDialog_Warning, Messages.get().CreateZoneDialog_WarningEmptyName);
			return;
		}
		
		super.okPressed();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

	/**
	 * Get zone UIN
	 * 
	 * @return zone UIN
	 */
	public int getZoneUIN()
	{
		return zoneUIN;
	}
}
