/**
 * 
 */
package org.netxms.webui.mobile.pages;

import java.io.IOException;
import java.util.List;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeContentProvider;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import com.eclipsesource.tabris.ui.AbstractPage;
import com.eclipsesource.tabris.ui.PageData;

/**
 * Predefined graph tree
 */
public class PredefinedGraphTree extends AbstractPage
{
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private TreeViewer viewer;
   
   /* (non-Javadoc)
    * @see com.eclipsesource.tabris.ui.AbstractPage#createContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public void createContent(Composite parent, PageData pageData)
   {
      parent.setLayout(new FillLayout());
      
      viewer = new TreeViewer(parent, SWT.NONE);
      viewer.setContentProvider(new GraphTreeContentProvider());
      viewer.setLabelProvider(new GraphTreeLabelProvider());

      reloadGraphList();
   }

   /**
    * Reload graph list from server
    */
   private void reloadGraphList()
   {
      try
      {
         final List<GraphSettings> list = session.getPredefinedGraphs();
         viewer.setInput(list);
      }
      catch(NXCException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
      catch(IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }
   
}
