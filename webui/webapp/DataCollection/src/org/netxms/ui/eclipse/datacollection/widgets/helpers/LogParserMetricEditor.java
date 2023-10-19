/**
 * NetXMS - open source network management system
 * Copyright (C) 2018-2022 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DashboardComposite;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Editor widget for single log parser metric
 */
public class LogParserMetricEditor extends DashboardComposite
{

   private LogParserMetric metric;
   private LogParserRuleEditor editor;

   private LabeledText labelMetricName;
   private Button checkPush;
   private Spinner pushGroup;

   /**
    * @param parent
    * @param style
    */
   public LogParserMetricEditor(Composite parent, LogParserMetric metric, final LogParserRuleEditor editor)
   {
      super(parent, SWT.BORDER);

      this.metric = metric;
      this.editor = editor;

      setBackground(parent.getBackground());

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = false;
      setLayout(layout);

      labelMetricName = new LabeledText(this, SWT.NONE);
      labelMetricName.setLabel("Metric name");
      labelMetricName.setBackground(getBackground());
      labelMetricName.setText(metric.getMetric());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      labelMetricName.setLayoutData(gd);
      labelMetricName.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });

      final Composite controlBar = new Composite(this, SWT.NONE);
      controlBar.setLayout(new RowLayout(SWT.HORIZONTAL));
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      controlBar.setLayoutData(gd);
      fillControlBar(controlBar);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = false;
      gd.verticalAlignment = SWT.BOTTOM;
      pushGroup = WidgetHelper.createLabeledSpinner(this, SWT.BORDER, "Capture group used for data push", 1, 500, gd);
      pushGroup.setSelection(metric.getGroup());
      pushGroup.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });

      checkPush = new Button(this, SWT.CHECK);
      checkPush.setBackground(getBackground());
      checkPush.setText("&Push");
      checkPush.setSelection(metric.isPush());
      checkPush.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editor.fireModifyListeners();
         }
      });
   }

   /**
    * Fill control bar
    * 
    * @param parent
    */
   private void fillControlBar(Composite parent)
   {
      ImageHyperlink link = new ImageHyperlink(parent, SWT.NONE);
      link.setImage(SharedIcons.IMG_DELETE_OBJECT);
      link.setToolTipText("Delete");
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editor.deleteMetric(metric);
         }
      });
   }

   /**
    * Save data from controls into parser rule
    */
   public void save()
   {
      metric.setGroup(pushGroup.getSelection());
      metric.setMetric(labelMetricName.getText());
      metric.setPush(checkPush.getSelection());
   }
}
