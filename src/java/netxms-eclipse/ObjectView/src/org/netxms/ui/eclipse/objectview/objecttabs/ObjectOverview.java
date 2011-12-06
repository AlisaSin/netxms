/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Layout;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.AvailabilityChart;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Capabilities;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Commands;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Comments;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Connection;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.GeneralInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedColors;

/**
 * Object overview tab
 */
public class ObjectOverview extends ObjectTab
{
	private NXCListener sessionListener = null;
	private Set<OverviewPageElement> elements = new HashSet<OverviewPageElement>();
	private ScrolledComposite scroller;
	private Composite viewArea;
	private Composite leftColumn;
	private Composite rightColumn;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
		scroller.getVerticalBar().setIncrement(20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = scroller.getClientArea();
				scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
			}
		});
		
		viewArea = new Composite(scroller, SWT.NONE);
		viewArea.setBackground(SharedColors.WHITE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		viewArea.setLayout(layout);
		scroller.setContent(viewArea);
		
		leftColumn = new Composite(viewArea, SWT.NONE);
		leftColumn.setLayout(createColumnLayout());
		leftColumn.setBackground(SharedColors.WHITE);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftColumn.setLayoutData(gd);
		
		rightColumn = new Composite(viewArea, SWT.NONE);
		rightColumn.setLayout(createColumnLayout());
		rightColumn.setBackground(SharedColors.WHITE);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
		gd.minimumWidth = SWT.DEFAULT;
		rightColumn.setLayoutData(gd);

		addElement(new GeneralInfo(leftColumn, getObject()));
		addElement(new Commands(leftColumn, getObject()));
		addElement(new AvailabilityChart(leftColumn, getObject()));
		addElement(new Comments(leftColumn, getObject()));
		addElement(new Capabilities(rightColumn, getObject()));
		addElement(new Connection(rightColumn, getObject()));
		
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
				{
					final GenericObject object = (GenericObject)n.getObject();
					if ((object != null) && (getObject() != null) && (object.getObjectId() == getObject().getObjectId()))
					{
						getViewPart().getSite().getShell().getDisplay().asyncExec(new Runnable() {
							@Override
							public void run()
							{
								changeObject(object);
							}
						});
					}
				}
			}
		};
		ConsoleSharedData.getSession().addListener(sessionListener);
	}
	
	/**
	 * Create layout for column
	 * 
	 * @return
	 */
	private Layout createColumnLayout()
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 5;
		return layout;
	}
	
	/**
	 * Add page element
	 * 
	 * @param element
	 * @param span
	 * @param grabExcessSpace
	 */
	private void addElement(OverviewPageElement element)
	{
		GridData gd = new GridData();
		gd.exclude = false;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		element.setLayoutData(gd);
		elements.add(element);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public void objectChanged(GenericObject object)
	{
		viewArea.setRedraw(false);
		for(OverviewPageElement element : elements)
		{
			if (element.isApplicableForObject(object))
			{
				element.setVisible(true);
				element.setObject(object);
			}
			else
			{
				element.setVisible(false);
			}
			((GridData)element.getLayoutData()).exclude = !element.isVisible();
		}
		viewArea.layout(true, true);
		viewArea.setRedraw(true);
		
		Rectangle r = scroller.getClientArea();
		scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
	 */
	@Override
	public void dispose()
	{
		if (sessionListener != null)
			ConsoleSharedData.getSession().removeListener(sessionListener);
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
	 */
	@Override
	public void selected()
	{
		super.selected();
		// I don't know why, but content lay out incorrectly if object selection
		// changes while this tab is not active.
		// As workaround, we force reconstruction of the content on each tab activation
		objectChanged(getObject());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		objectChanged(getObject());
	}
}
