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
package org.netxms.ui.eclipse.console.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Edit key/value pair
 */
public class KeyValuePairEditDialog extends Dialog
{
	private LabeledText textName;
	private LabeledText textValue;
	private String pStorageKey;
	private String pStorageValue;
	private String label;
	private boolean showValue;
   private boolean isNew;
	
	/**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param key key being edited
    * @param value current value
    * @param showValue true if value should be shown in dialog
    * @param isNew true if new element is being created
    */
	public KeyValuePairEditDialog(Shell parentShell, String key, String value, boolean showValue, boolean isNew, String label)
	{
		super(parentShell);
		pStorageKey = key;
		pStorageValue = value;
		this.showValue = showValue;
		this.isNew = isNew;
		this.label = label;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
		
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(label);
      textName.getTextControl().setTextLimit(256);
      if (pStorageKey != null)
      {
      	textName.setText(pStorageKey);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      if (showValue && !isNew)
         textName.setEditable(false);

      if (showValue)
      {
         textValue = new LabeledText(dialogArea, SWT.NONE);
         textValue.setLabel("Value");
         if (pStorageValue != null)
         	textValue.setText(pStorageValue);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         textValue.setLayoutData(gd);

         if (pStorageKey != null)
         	textValue.setFocus();
      }

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(isNew ? "Create Value" : "Change Value");
	}

	/**
	 * Get attribute name
	 * 
	 */
	public String getAtributeName()
	{
		return pStorageKey;
	}

	/**
	 * Get attribute value
	 * 
	 */
	public String getAttributeValue()
	{
		return pStorageValue;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		pStorageKey = textName.getText();
      if (showValue)
		   pStorageValue = textValue.getText();
		super.okPressed();
	}
}
