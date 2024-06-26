/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.ObjectTreeViewer;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Object status indicator
 */
public class ObjectStatusIndicator extends Canvas implements PaintListener
{
   private I18n i18n = LocalizationHelper.getI18n(ObjectStatusIndicator.class);
	private ObjectTreeViewer objectTree = null;
	private boolean showIcons = false;
	private boolean hideNormal = true;
	private boolean hideUnmanaged = true;
	private boolean hideUnknown = true;
	private boolean hideDisabled = true;
	private Action actionShowIcons;
	private Action actionHideNormal;
	private Action actionHideUnmanaged;
	private Action actionHideUnknown;
	private Action actionHideDisabled;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectStatusIndicator(Composite parent, int style)
	{
		super(parent, style | SWT.DOUBLE_BUFFERED);
		addPaintListener(this);
		setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
		
      final PreferenceStore ps = PreferenceStore.getInstance();
      showIcons = ps.getAsBoolean("ObjectStatusIndicator.showIcons", false); //$NON-NLS-1$
      hideNormal = ps.getAsBoolean("ObjectStatusIndicator.hideNormal", true); //$NON-NLS-1$
      hideUnmanaged = ps.getAsBoolean("ObjectStatusIndicator.hideUnmanaged", true); //$NON-NLS-1$
      hideUnknown = ps.getAsBoolean("ObjectStatusIndicator.hideUnknown", true); //$NON-NLS-1$
      hideDisabled = ps.getAsBoolean("ObjectStatusIndicator.hideDisabled", true); //$NON-NLS-1$
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
		      removePaintListener(ObjectStatusIndicator.this);
            ps.set("ObjectStatusIndicator.showIcons", showIcons); //$NON-NLS-1$
            ps.set("ObjectStatusIndicator.hideNormal", hideNormal); //$NON-NLS-1$
            ps.set("ObjectStatusIndicator.hideUnmanaged", hideUnmanaged); //$NON-NLS-1$
            ps.set("ObjectStatusIndicator.hideUnknown", hideUnknown); //$NON-NLS-1$
            ps.set("ObjectStatusIndicator.hideDisabled", hideDisabled); //$NON-NLS-1$
			}
		});

		createActions();

		MenuManager manager = new MenuManager();
		manager.setRemoveAllWhenShown(true);
		manager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});
		setMenu(manager.createContextMenu(this));
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionShowIcons = new Action(i18n.tr("Show status &icons"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				showIcons = actionShowIcons.isChecked();
				redraw();
			}
		};
		actionShowIcons.setChecked(showIcons);

      actionHideDisabled = new Action(i18n.tr("Hide &disabled objects"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				hideDisabled = actionHideDisabled.isChecked();
				redraw();
			}
		};
		actionHideDisabled.setChecked(hideDisabled);

      actionHideNormal = new Action(i18n.tr("Hide &normal objects"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				hideNormal = actionHideNormal.isChecked();
				redraw();
			}
		};
		actionHideNormal.setChecked(hideNormal);

      actionHideUnknown = new Action(i18n.tr("Hide &unknown objects"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				hideUnknown = actionHideUnknown.isChecked();
				redraw();
			}
		};
		actionHideUnknown.setChecked(hideUnknown);

      actionHideUnmanaged = new Action(i18n.tr("Hide un&managed objects"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				hideUnmanaged = actionHideUnmanaged.isChecked();
				redraw();
			}
		};
		actionHideUnmanaged.setChecked(hideUnmanaged);
	}

	/**
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionShowIcons);
		manager.add(new Separator());
		manager.add(actionHideNormal);
		manager.add(actionHideUnknown);
		manager.add(actionHideUnmanaged);
		manager.add(actionHideDisabled);
	}

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(21, 10);
	}

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
	@Override
	public void paintControl(PaintEvent e)
	{
		if (objectTree == null)
			return;
		
		final GC gc = e.gc;
		gc.setAntialias(SWT.ON);
		final int width = getClientArea().width;
		
		TreeItem item = objectTree.getTree().getTopItem();
		if (item != null)
		{		
			int y = 0;
			final int limit = objectTree.getTree().getClientArea().height;
			final int height = objectTree.getTree().getItemHeight();
	
			ViewerRow row = objectTree.getTreeViewerRow(item);
			while((row != null) && (y < limit))
			{
				AbstractObject object = (AbstractObject)row.getItem().getData();
				drawObject(gc, object, y, width, height);
				y += height;
				row = row.getNeighbor(ViewerRow.BELOW, false);
				if (row == null)
				{
					// sometimes ViewerRow.getNeighbor returns null in the middle of the tree
					// this usually happens when filter is applied
					// in that case we try to find next item from position
					for(int i = 16; i < 192; i+= 16)
					{
						item = objectTree.getTree().getItem(new Point(y, y + height / 2));
						if (item != null)
						{
							row = objectTree.getTreeViewerRow(item);
							break;
						}
					}
				}
			}
		}		
      gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_NORMAL_SHADOW));
		gc.drawLine(width - 1, 0, width - 1, getClientArea().height);
	}
	
	/**
	 * @param object
	 * @param y
	 */
	private void drawObject(GC gc, AbstractObject object, int y, int width, int height)
	{
		final ObjectStatus status = object.getStatus();
		
		if ((status == ObjectStatus.NORMAL) && hideNormal)
			return;
		if ((status == ObjectStatus.UNMANAGED) && hideUnmanaged)
			return;
		if ((status == ObjectStatus.UNKNOWN) && hideUnknown)
			return;
		if ((status == ObjectStatus.DISABLED) && hideDisabled)
			return;
		
		if (showIcons)
		{
			gc.drawImage(StatusDisplayInfo.getStatusImage(status), (width - 16) / 2, y + (height - 16) / 2);
		}
		else
		{
			gc.setBackground(StatusDisplayInfo.getStatusColor(status));
			gc.setForeground(StatusDisplayInfo.getStatusColor(status));
			int size = Math.min(width - 8, height - 8);
			gc.setAlpha(127);
			gc.fillOval((width - size) / 2, y + (height - size) / 2, size, size);
			gc.setAlpha(255);
			gc.drawOval((width - size) / 2, y + (height - size) / 2, size, size);
		}
	}

	/**
	 * Refresh control
	 * 
	 * @param objectTree
	 */
	public void refresh(ObjectTreeViewer objectTree)
	{
		this.objectTree = objectTree;
		redraw();
	}
}
