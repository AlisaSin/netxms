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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.ThresholdStateChange;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeContentProvider;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.VisibilityValidator;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Widget to show threshold violation summary
 */
public class ThresholdSummaryWidget extends CompositeWithMessageBar
{
	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_STATUS = 1;
	public static final int COLUMN_PARAMETER = 2;
	public static final int COLUMN_VALUE = 3;
	public static final int COLUMN_CONDITION = 4;
   public static final int COLUMN_EVENT = 5;
   public static final int COLUMN_TIMESTAMP = 6;

	private AbstractObject object;
	private IViewPart viewPart;
	private SortableTreeViewer viewer;
	private VisibilityValidator visibilityValidator;
	private boolean subscribed = false;
	private boolean refreshScheduled = false;

	/**
	 * @param parent
	 * @param style
	 */
	public ThresholdSummaryWidget(Composite parent, int style, IViewPart viewPart, VisibilityValidator visibilityValidator)
	{
		super(parent, style);
		this.viewPart = viewPart;
		this.visibilityValidator = visibilityValidator;

      final String[] names = { Messages.get().ThresholdSummaryWidget_Node, Messages.get().ThresholdSummaryWidget_Status, Messages.get().ThresholdSummaryWidget_Parameter,
            Messages.get().ThresholdSummaryWidget_Value, Messages.get().ThresholdSummaryWidget_Condition, "Event", Messages.get().ThresholdSummaryWidget_Since };
      final int[] widths = { 200, 100, 250, 100, 100, 250, 140 };
		viewer = new SortableTreeViewer(getContent(), names, widths, COLUMN_NODE, SWT.UP, SWT.FULL_SELECTION);
		viewer.setContentProvider(new ThresholdTreeContentProvider());
		viewer.setLabelProvider(new ThresholdTreeLabelProvider());		
		viewer.setComparator(new ThresholdTreeComparator());

		createPopupMenu();

		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (!subscribed)
               return;

            final NXCSession session = ConsoleSharedData.getSession();
            ConsoleJob job = new ConsoleJob("Unsubscribe from threshold notifications", null, Activator.PLUGIN_ID) {
               @Override
               protected void runInternal(IProgressMonitor monitor) throws Exception
               {
                  session.unsubscribe(NXCSession.CHANNEL_DC_THRESHOLDS);
               }

               @Override
               protected String getErrorMessage()
               {
                  return "Cannot change event subscription";
               }
            };
            job.setUser(false);
            job.setSystem(true);
            job.start();
         }
      });

		ConsoleSharedData.getSession().addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.THRESHOLD_STATE_CHANGED)
            {
               final ThresholdStateChange stateChange = (ThresholdStateChange)n.getObject();
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     processNotification(stateChange);
                  }
               });
            }
         }
      });
	}

	/**
	 * Create pop-up menu for alarm list
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
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, viewer);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	
	/**
	 * Process threshold state change
	 * 
	 * @param stateChange state change information
	 */
	private void processNotification(ThresholdStateChange stateChange)
	{
	   if (refreshScheduled || (object == null) ||
	       ((object.getObjectId() != stateChange.getObjectId()) && !object.isParentOf(stateChange.getObjectId())))
	      return;

	   refreshScheduled = true;
	   getDisplay().timerExec(500, new Runnable() {
         @Override
         public void run()
         {
            refreshScheduled = false;
            refresh();
         }
      });
	}

	/**
	 * Refresh widget
	 */
	public void refresh()
	{
	   if (visibilityValidator != null && !visibilityValidator.isVisible())
	      return;
	   
		if (object == null)
		{
			viewer.setInput(new ArrayList<ThresholdViolationSummary>(0));
			return;
		}
		
		final NXCSession session = ConsoleSharedData.getSession();
		final long rootId = object.getObjectId();
		ConsoleJob job = new ConsoleJob(Messages.get().ThresholdSummaryWidget_JobTitle, viewPart, Activator.PLUGIN_ID, this) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ThresholdViolationSummary> data = session.getThresholdSummary(rootId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (isDisposed() || (object == null) || (rootId != object.getObjectId()))
							return;
						viewer.setInput(data);
						viewer.expandAll();
					}
				});
				if (!subscribed)
				{
				   session.subscribe(NXCSession.CHANNEL_DC_THRESHOLDS);
				   subscribed = true;
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ThresholdSummaryWidget_JobError;
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @param object the object to set
	 */
	public void setObject(AbstractObject object)
	{
		this.object = object;
		refresh();
	}
}
