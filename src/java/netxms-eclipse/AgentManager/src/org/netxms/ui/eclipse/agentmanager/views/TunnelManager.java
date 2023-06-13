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

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
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
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AgentTunnel;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.TunnelListComparator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.TunnelListLabelProvider;
import org.netxms.ui.eclipse.agentmanager.views.helpers.TunnelManagerFilter;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectmanager.dialogs.CreateNodeDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Tunnel manager view
 */
public class TunnelManager extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.TunnelManager";
   
   public static final int COL_ID = 0;
   public static final int COL_STATE = 1;
   public static final int COL_NODE = 2;
   public static final int COL_IP_ADDRESS = 3;
   public static final int COL_CHANNELS = 4;
   public static final int COL_SYSNAME = 5;
   public static final int COL_HOSTNAME = 6;
   public static final int COL_PLATFORM = 7;
   public static final int COL_SYSINFO = 8;
   public static final int COL_HARDWARE_ID = 9;
   public static final int COL_SERIAL_NUMBER = 10;
   public static final int COL_AGENT_VERSION = 11;
   public static final int COL_AGENT_ID = 12;
   public static final int COL_AGENT_PROXY = 13;
   public static final int COL_SNMP_PROXY = 14;
   public static final int COL_SNMP_TRAP_PROXY = 15;
   public static final int COL_SYSLOG_PROXY = 16;
   public static final int COL_USER_AGENT = 17;
   public static final int COL_CERTIFICATE_EXPIRATION = 18;
   public static final int COL_CONNECTION_TIME = 19;

   private NXCSession session = ConsoleSharedData.getSession();
   private Map<Integer, AgentTunnel> tunnels = new HashMap<>();
   private Display display = Display.getCurrent();
   private SortableTableViewer viewer;
   private TunnelManagerFilter filter;
   private boolean initShowfilter = true;
   private FilterText filterText;
   private Action actionRefresh;
   private Action actionCreateNode;
   private Action actionBind;
   private Action actionUnbind;
   private Action actionShowFilter;
   private Action actionHideNonProxy;
   private Action actionHideNonUA;
   
   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FormLayout());

      // Create filter area
      filterText = new FilterText(parent, SWT.NONE, null, true);
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
            actionShowFilter.setChecked(false);
         }
      });

      final String[] names = 
         { "ID", "State", "Node", "IP address", "Channels", "System name", "Hostname",
           "Platform", "System information", "Hardware ID", "Serial number", "Agent version",
           "Agent ID", "Agent proxy", "SNMP proxy", "SNMP trap proxy", "Syslog proxy", "User agent", 
           "Certificate expiration", "Connection time" };
      final int[] widths = { 80, 80, 140, 150, 80, 150, 150, 250, 300, 180, 150, 150, 150, 80, 80, 80, 80, 80, 130, 130 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TunnelListLabelProvider());
      viewer.setComparator(new TunnelListComparator());
      filter = new TunnelManagerFilter();
      viewer.addFilter(filter);

      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      initShowfilter = settings.getBoolean(ID + "initShowFilter");
      
      WidgetHelper.restoreTableViewerSettings(viewer, settings, "TunnelManager");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, settings, "TunnelManager");
            settings.put(ID + "initShowFilter", initShowfilter);
         }
      });
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
      
      createActions();
      contributeToActionBars();
      createPopupMenu();
      activateContext();

      // Set initial focus to filter input line
      if (initShowfilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      refresh();

      session.addListener(this);
      new ConsoleJob("Subscribing to tunnel change notifications", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_AGENT_TUNNELS);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot subscribe to tunnel change notifications";
         }
      }.start();
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.agentmanager.context.TunnelManager"); //$NON-NLS-1$
      }
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      new ConsoleJob("Unsubscribing from tunnel change notifications", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            try
            {
               session.unsubscribe(NXCSession.CHANNEL_AGENT_TUNNELS);
            }
            catch(Exception e)
            {
               Activator.logError("Cannot remove subscription for agent tunnel notifications", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      }.start();
      super.dispose();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionCreateNode = new Action("&Create node and bind...") {
         @Override
         public void run()
         {
            createNode();
         }
      };
      
      actionBind = new Action("&Bind to...") {
         @Override
         public void run()
         {
            bindTunnel();
         }
      };
      
      actionUnbind = new Action("&Unbind") {
         @Override
         public void run()
         {
            unbindTunnel();
         }
      };
      
      actionShowFilter = new Action("&Show filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowfilter);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.agentmanager.commands.show_filter"); //$NON-NLS-1$
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));

      actionHideNonProxy = new Action("Hide tunnels without proxy function", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setHideNonProxy(actionHideNonProxy.isChecked());
         }
      };

      actionHideNonUA = new Action("Hide tunnels without user agent", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setHideNonUA(actionHideNonUA.isChecked());
         }
      };
   }

   /**
    * Contribute actions to action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pulldown menu
    * @param manager menu manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionShowFilter);
      manager.add(actionHideNonProxy);
      manager.add(actionHideNonUA);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local toolbar
    * @param manager menu manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionRefresh);
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
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension.
      getSite().setSelectionProvider(viewer);
      getSite().registerContextMenu(menuMgr, viewer);
   }
   
   /**
    * Fill context menu
    * @param manager Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() == 1) && !((AgentTunnel)selection.getFirstElement()).isBound())
      {
         manager.add(actionBind);
         manager.add(actionCreateNode);
      }
      else
      {
         for(Object o : selection.toList())
         {
            if (((AgentTunnel)o).isBound())
            {
               manager.add(actionUnbind);
               break;
            }
         }
      }
      manager.add(actionShowFilter);
   }
   
   /**
    * Refresh view
    */
   private void refresh()
   {
      new ConsoleJob("Get list of active agent tunnels", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<AgentTunnel> tunnelList = session.getAgentTunnels();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  tunnels.clear();
                  for(AgentTunnel t : tunnelList)
                     tunnels.put(t.getId(), t);
                  viewer.setInput(tunnels.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of active agent tunnels";
         }
      }.start();
   }

   /**
    * Create new node and bind tunnel
    */
   private void createNode()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentTunnel tunnel = (AgentTunnel)selection.getFirstElement();
      if (tunnel.isBound())
         return;

      CreateNodeDialog dlg = new CreateNodeDialog(getSite().getShell(), null);
      dlg.setEnableShowAgainFlag(false);
      dlg.setObjectName(tunnel.getSystemName());
      dlg.setZoneUIN(tunnel.getZoneUIN());
      if (dlg.open() != Window.OK)
         return;

      final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, dlg.getObjectName(), 2);
      cd.setCreationFlags(dlg.getCreationFlags());
      cd.setPrimaryName(dlg.getHostName());
      cd.setObjectAlias(dlg.getObjectAlias());
      cd.setAgentPort(dlg.getAgentPort());
      cd.setAgentProxyId(dlg.getAgentProxy());
      cd.setSnmpPort(dlg.getSnmpPort());
      cd.setSnmpProxyId(dlg.getSnmpProxy());
      cd.setIcmpProxyId(dlg.getIcmpProxy());
      cd.setEtherNetIpPort(dlg.getEtherNetIpPort());
      cd.setEtherNetIpProxyId(dlg.getEtherNetIpProxy());
      cd.setModbusTcpPort(dlg.getModbusTcpPort());
      cd.setModbusUnitId(dlg.getModbusUnitId());
      cd.setModbusProxyId(dlg.getModbusProxy());
      cd.setWebServiceProxyId(dlg.getWebServiceProxy());
      cd.setSshPort(dlg.getSshPort());
      cd.setSshProxyId(dlg.getSshProxy());
      cd.setSshLogin(dlg.getSshLogin());
      cd.setSshPassword(dlg.getSshPassword());
      cd.setMqttProxyId(dlg.getMqttProxy());
      cd.setZoneUIN(dlg.getZoneUIN());

      new ConsoleJob("Create new node and bind tunnel", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            // Always create node as unmanaged to start configuration poll only after tunnel bind
            boolean stayUnmanaged = ((cd.getCreationFlags() & NXCObjectCreationData.CF_CREATE_UNMANAGED) != 0);
            cd.setCreationFlags(cd.getCreationFlags() | NXCObjectCreationData.CF_CREATE_UNMANAGED);

            long nodeId = session.createObject(cd);
            session.bindAgentTunnel(tunnel.getId(), nodeId);

            if (!stayUnmanaged)
            {
               // Wait for tunnel to appear
               session.waitForAgentTunnel(nodeId, 20000);
               session.setObjectManaged(nodeId, true);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot create node and bind tunnel";
         }
      }.start();
   }

   /**
    * Bind tunnel to node
    */
   private void bindTunnel()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentTunnel tunnel = (AgentTunnel)selection.getFirstElement();
      if (tunnel.isBound())
         return;

      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
      if (dlg.open() != Window.OK)
         return;      
      final long nodeId = dlg.getSelectedObjects().get(0).getObjectId();

      new ConsoleJob("Bind tunnels", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.bindAgentTunnel(tunnel.getId(), nodeId);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot bind tunnel";
         }
      }.start();
   }

   /**
    * Unbind tunnel
    */
   private void unbindTunnel()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Unbind Tunnel", "Selected tunnels will be unbound. Are you sure?"))
         return;

      final Object[] tunnels = selection.toArray();
      new ConsoleJob("Unbind tunnels", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object o : tunnels)
            {
               AgentTunnel t = (AgentTunnel)o;
               if (!t.isBound())
                  continue;
               session.unbindAgentTunnel(t.getNodeId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot unbind tunnel";
         }
      }.start();
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      initShowfilter = enable;
      filterText.setVisible(initShowfilter);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      viewer.getTable().getParent().layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   private void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
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
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.AGENT_TUNNEL_OPEN)
      {
         display.asyncExec(new Runnable() {
            @Override
            public void run()
            {
               AgentTunnel t = (AgentTunnel)n.getObject();
               tunnels.put(t.getId(), t);
               viewer.refresh();
            }
         });
      }
      else if (n.getCode() == SessionNotification.AGENT_TUNNEL_CLOSED)
      {
         display.asyncExec(new Runnable() {
            @Override
            public void run()
            {
               tunnels.remove((int)n.getSubCode());
               viewer.refresh();
            }
         });
      }
   }
}
