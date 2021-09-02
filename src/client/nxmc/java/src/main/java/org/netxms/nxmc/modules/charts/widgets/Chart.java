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
package org.netxms.nxmc.modules.charts.widgets;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.api.DataSeries;
import org.netxms.nxmc.tools.ColorCache;

/**
 * Generic chart widget
 */
public class Chart extends Composite
{
   private ChartType type = ChartType.LINE;
   private ChartConfiguration configuration;
   protected ChartColor[] palette = null;
   private List<GraphItem> items = new ArrayList<GraphItem>(ChartConfiguration.MAX_GRAPH_ITEM_COUNT);
   private List<DataSeries> dataSeries = new ArrayList<DataSeries>(ChartConfiguration.MAX_GRAPH_ITEM_COUNT);
   private long drillDownObjectId = 0;
   private ColorCache colorCache;
   private Label title;
   private ChartLegend legend;
   private Composite plotAreaComposite;
   private PlotArea plotArea;
   private boolean mouseDown = false;
   private Set<IDoubleClickListener> doubleClickListeners = new HashSet<IDoubleClickListener>();

   /**
    * Create empty chart control.
    *
    * @param parent parent control
    * @param style chart control style
    */
   public Chart(Composite parent, int style)
   {
      this(parent, style, ChartType.LINE, null);
   }

   /**
    * Create chart control with given configuration.
    *
    * @param parent parent control
    * @param style chart control style
    */
   public Chart(Composite parent, int style, ChartType type, ChartConfiguration configuration)
   {
      super(parent, style);

      colorCache = new ColorCache(this);

      createDefaultPalette();
      setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$

      this.type = type;
      this.configuration = configuration;
      if (configuration != null)
         rebuild();

      addMouseListener(new MouseListener() {
         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               mouseDown = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && mouseDown)
            {
               mouseDown = false;
               if (drillDownObjectId != 0)
                  openDrillDownObject();
            }
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            mouseDown = false;
            fireDoubleClickListeners();
         }
      });
   }

   /**
    * Create color object from preference string
    * 
    * @param name Preference name
    * @return Color object
    */
   protected Color getColorFromPreferences(final String name)
   {
      return colorCache.create(PreferenceStore.getInstance().getAsColor(name));
   }

   /**
    * Get chart type
    *
    * @return chart type
    */
   public ChartType getType()
   {
      return type;
   }

   /**
    * Set chart type
    *
    * @param type new chart type
    */
   public void setType(ChartType type)
   {
      this.type = type;
   }

   /**
    * Check if current chart type has axes.
    *
    * @return true if current chart type has axes
    */
   public boolean hasAxes()
   {
      return type != ChartType.PIE;
   }

   /**
    * Get metrics
    *
    * @return lis of metrics
    */
   protected List<GraphItem> getItems()
   {
      return items;
   }

   /**
    * Get specific item
    *
    * @param index item index
    * @return item or null
    */
   public GraphItem getItem(int index)
   {
      try
      {
         return items.get(index);
      }
      catch(ArrayIndexOutOfBoundsException e)
      {
         return null;
      }
   }

   /**
    * Get number of items on chart.
    *
    * @return number of items on chart
    */
   public int getItemCount()
   {
      return items.size();
   }

   /**
    * Get data series
    *
    * @return list of data series
    */
   protected List<DataSeries> getDataSeries()
   {
      return dataSeries;
   }

   /**
    * Get chart's color cache.
    *
    * @return chart's color cache
    */
   protected ColorCache getColorCache()
   {
      return colorCache;
   }

   /**
    * Get current chart configuration
    *
    * @return current chart configuration
    */
   public ChartConfiguration getConfiguration()
   {
      return configuration;
   }

   /**
    * Re-create chart with new configuration.
    *
    * @param configuration new configuration
    */
   public void reconfigure(ChartConfiguration configuration)
   {
      this.configuration = configuration;
      rebuild();
   }

   /**
    * Re-create chart with current configuration.
    */
   public void rebuild()
   {
      if (plotAreaComposite != null)
         plotAreaComposite.dispose();
      if (title != null)
         title.dispose();
      if (legend != null)
         legend.dispose();

      if (configuration != null)
      {
         GridLayout layout = new GridLayout();
         layout.numColumns = isLegendOnSide() ? 2 : 1;
         layout.marginWidth = 5;
         layout.marginHeight = 5;
         setLayout(layout);

         if (configuration.isTitleVisible())
         {
            createTitle();
         }
         else
         {
            title = null;
         }

         if (configuration.isLegendVisible() && ((configuration.getLegendPosition() == ChartConfiguration.POSITION_LEFT) || (configuration.getLegendPosition() == ChartConfiguration.POSITION_TOP)))
            createLegend();
         createPlotArea();
         if (configuration.isLegendVisible() && ((configuration.getLegendPosition() == ChartConfiguration.POSITION_RIGHT) || (configuration.getLegendPosition() == ChartConfiguration.POSITION_BOTTOM)))
            createLegend();
      }
      else
      {
         plotAreaComposite = null;
         plotArea = null;
         title = null;
         legend = null;
      }

      layout(true, true);
   }

   /**
    * Create chart title widget
    *
    * @param insertBefore insert before given control
    */
   private void createTitle()
   {
      title = new Label(this, SWT.NONE);
      title.setBackground(getBackground());
      title.setText(configuration.getTitle());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.horizontalSpan = isLegendOnSide() ? 2 : 1;
      title.setLayoutData(gd);
   }

   /**
    * Create legend widget
    */
   private void createLegend()
   {
      legend = new ChartLegend(this, isLegendOnSide());
      GridData gd = new GridData();
      if (isLegendOnSide())
      {
         gd.horizontalAlignment = SWT.CENTER;
         gd.verticalAlignment = SWT.TOP;
         gd.grabExcessVerticalSpace = true;
      }
      else
      {
         gd.horizontalAlignment = SWT.LEFT;
         gd.verticalAlignment = SWT.CENTER;
         gd.grabExcessHorizontalSpace = true;
      }
      legend.setLayoutData(gd);
   }

   /**
    * Check if legend is visible and is located on left or right size
    *
    * @return true if legend is visible and is located on left or right size
    */
   private boolean isLegendOnSide()
   {
      int legendPosition = configuration.getLegendPosition();
      return configuration.isLegendVisible() && ((legendPosition == ChartConfiguration.POSITION_LEFT) || (legendPosition == ChartConfiguration.POSITION_RIGHT));
   }

   /**
    * Create plot area
    */
   private void createPlotArea()
   {
      switch(type)
      {
         case BAR:
            plotAreaComposite = new BarChart(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case DIAL:
            plotAreaComposite = new DialGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case GAUGE:
            plotAreaComposite = new BarGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case LINE:
            plotAreaComposite = new LineChart(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case PIE:
            plotAreaComposite = new PieChart(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case TEXT:
            plotAreaComposite = new TextGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         default:
            plotAreaComposite = new Composite(this, SWT.NONE);
            plotAreaComposite.setBackground(getBackground());
            plotArea = null;
      }
      plotAreaComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
   }

   /**
    * Set palette.
    * 
    * @param colors colors for series or null to set default
    */
   public void setPalette(ChartColor[] colors)
   {
      if (colors != null)
         palette = colors;
      else
         createDefaultPalette();
   }

   /**
    * Get single palette element.
    * 
    * @param index element index
    */
   public ChartColor getPaletteEntry(int index)
   {
      try
      {
         return palette[index];
      }
      catch(ArrayIndexOutOfBoundsException e)
      {
         return null;
      }
   }

   /**
    * Set single palette element.
    * 
    * @param index element index
    * @param color color for series
    */
   public void setPaletteEntry(int index, ChartColor color)
   {
      try
      {
         palette[index] = color;
      }
      catch(ArrayIndexOutOfBoundsException e)
      {
      }
   }

   /**
    * Create default palette from preferences
    */
   protected void createDefaultPalette()
   {
      palette = new ChartColor[ChartConfiguration.MAX_GRAPH_ITEM_COUNT];
      for(int i = 0; i < ChartConfiguration.MAX_GRAPH_ITEM_COUNT; i++)
      {
         palette[i] = ChartColor.getDefaultColor(i);
      }
   }

   /**
    * Add metric
    * 
    * @param parameter DCI information
    * @param value parameter's initial value
    * @return parameter's index (0 .. MAX_CHART_ITEMS-1)
    */
   public int addParameter(GraphItem metric)
   {
      if (items.size() >= ChartConfiguration.MAX_GRAPH_ITEM_COUNT)
         return -1;

      items.add(metric);
      dataSeries.add(new DataSeries());
      return items.size() - 1;
   }

   /**
    * Remove all metrics from chart
    */
   public void removeAllParameters()
   {
      items.clear();
      dataSeries.clear();
   }

   /**
    * Update values for parameter
    * 
    * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
    * @param value parameter's value
    * @param updateChart if true, chart will be updated (repainted)
    */
   public void updateParameter(int index, DciData values, boolean updateChart)
   {
      dataSeries.set(index, new DataSeries(values));
      if (updateChart)
         refresh();
   }

   /**
    * Update value for parameter
    * 
    * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
    * @param value parameter's value
    * @param updateChart if true, chart will be updated (repainted)
    */
   public void updateParameter(int index, double value, boolean updateChart)
   {
      dataSeries.set(index, new DataSeries(value));
      if (updateChart)
         refresh();
   }

   /**
    * Update value for parameter
    * 
    * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
    * @param value parameter's value
    * @param dataType DCI data type
    * @param updateChart if true, chart will be updated (repainted)
    */
   public void updateParameter(int index, DciDataRow value, DataType dataType, boolean updateChart)
   {
      dataSeries.set(index, new DataSeries(value, dataType));
      if (updateChart)
         refresh();
   }

   /**
    * Update thresholds for parameter
    * 
    * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
    * @param thresholds new thresholds
    */
   public void updateParameterThresholds(int index, Threshold[] thresholds)
   {
      // FIXME: implement
   }

   /**
    * Set time range for chart.
    * 
    * @param from start time
    * @param to end time
    */
   public void setTimeRange(final Date from, final Date to)
   {
      if (plotArea instanceof LineChart)
         ((LineChart)plotArea).setTimeRange(from, to);
   }

   /**
    * Add information about error. Chart implementation should display error notification in some way.
    * 
    * @param message error message
    */
   public void addError(String message)
   {
      // FIXME: implement
   }

   /**
    * Clear errors
    */
   public void clearErrors()
   {
      // FIXME: implement
   }

   /**
    * Adjust X axis to fit all data
    * 
    * @param repaint if true, chart will be repainted after change
    */
   public void adjustXAxis(boolean repaint)
   {
      // FIXME: implement
   }

   /**
    * Adjust Y axis to fit all data
    * 
    * @param repaint if true, chart will be repainted after change
    */
   public void adjustYAxis(boolean repaint)
   {
      // FIXME: implement
   }

   /**
    * Zoom in
    */
   public void zoomIn()
   {
      // FIXME: implement
   }

   /**
    * Zoom out
    */
   public void zoomOut()
   {
      // FIXME: implement
   }

   /**
    * Refresh (repaint) chart using current data and settings
    */
   public void refresh()
   {
      if (configuration.isLegendVisible() && configuration.isExtendedLegend())
         legend.refresh();
      if (plotArea != null)
         plotArea.refresh();
   }

   /**
    * Get ID of drill-down object for this gauge (dashboard or network map)
    */
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }

   /**
    * Set ID of drill-down object for this gauge (dashboard or network map)
    * 
    * @param objectId ID of drill-down object or 0 to disable drill-down functionality
    */
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
      setCursor(getDisplay().getSystemCursor((drillDownObjectId != 0) ? SWT.CURSOR_HAND : SWT.CURSOR_ARROW));
   }

   /**
    * Open drill-down object
    */
   void openDrillDownObject()
   {
      AbstractObject object = Registry.getSession().findObjectById(drillDownObjectId);
      if (object == null)
         return;

      if (!(object instanceof Dashboard) && !(object instanceof NetworkMap))
         return;

      /* TODO: implement open */
   }

   /**
    * Add double click listener. Listener will be called on double click on any chart element - plot area, legend, or title.
    *
    * @param listener double click listener
    */
   public void addDoubleClickListener(IDoubleClickListener listener)
   {
      doubleClickListeners.add(listener);
   }

   /**
    * Remove previously added double click listener.
    *
    * @param listener double click listener
    */
   public void removeDoubleClickListener(IDoubleClickListener listener)
   {
      doubleClickListeners.add(listener);
   }

   /**
    * Fire registered double click listeners
    */
   private void fireDoubleClickListeners()
   {
      for(IDoubleClickListener l : doubleClickListeners)
         l.doubleClick(null);
   }

   /**
    * Take snapshot of this chart.
    *
    * @return image containing snapshot of this chart
    */
   public Image takeSnapshot()
   {
      Rectangle rect = getClientArea();
      Image image = new Image(getDisplay(), rect.width, rect.height);
      GC gc = new GC(image);
      this.print(gc);
      gc.dispose();
      return image;
   }
}
