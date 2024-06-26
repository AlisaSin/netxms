package org.netxms.ui.android.main.activities;

import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.TextView;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.ui.android.R;
import org.netxms.ui.android.helpers.Colors;
import org.netxms.ui.android.main.adapters.GraphAdapter;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Browser for predefined graphics
 *
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 */
public class GraphBrowser extends AbstractClientActivity {
    private static final String TAG = "nxclient/GraphBrowser";
    private GraphAdapter adapter;
    private ProgressDialog dialog;

    /*
     * (non-Javadoc)
     *
     * @see
     * org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2
     * (android.os.Bundle)
     */
    @Override
    protected void onCreateStep2(Bundle savedInstanceState) {
        dialog = new ProgressDialog(this);
        setContentView(R.layout.graph_view);

        TextView title = findViewById(R.id.ScreenTitlePrimary);
        title.setText(R.string.predefined_graphs_title);

        // keeps current list of graphs as datasource for listview
        adapter = new GraphAdapter(this, new ArrayList<String>(), new ArrayList<ArrayList<GraphDefinition>>());
        ExpandableListView listView = findViewById(R.id.GraphList);
        listView.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {
            @Override
            public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id) {
                GraphDefinition gs = (GraphDefinition) adapter.getChild(groupPosition, childPosition);
                if (gs != null) {
                    drawGraph(gs);
                    return true;
                }
                return false;
            }
        });
        listView.setAdapter(adapter);
        registerForContextMenu(listView);
    }

    /*
     * (non-Javadoc)
     *
     * @see android.content.ServiceConnection#onServiceConnected(android.content.
     * ComponentName, android.os.IBinder)
     */
    @Override
    public void onServiceConnected(ComponentName name, IBinder binder) {
        super.onServiceConnected(name, binder);
        service.registerGraphBrowser(this);
        refreshList();
    }

    /*
     * (non-Javadoc)
     *
     * @see
     * android.content.ServiceConnection#onServiceDisconnected(android.content
     * .ComponentName)
     */
    @Override
    public void onServiceDisconnected(ComponentName name) {
        super.onServiceDisconnected(name);
    }

    /**
     * Draw graph based on stored settings
     */
    public void drawGraph(GraphDefinition gs) {
        if (gs != null) {
            Intent newIntent = new Intent(this, DrawGraph.class);
            ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
            ArrayList<Integer> dciIdList = new ArrayList<Integer>();
            ArrayList<Integer> colorList = new ArrayList<Integer>();
            ArrayList<Integer> lineWidthList = new ArrayList<Integer>();
            ArrayList<String> nameList = new ArrayList<String>();

            // Set values
            ChartDciConfig[] items = gs.getDciList();
            for (int i = 0; i < items.length && i < Colors.DEFAULT_ITEM_COLORS.length; i++) {
                nodeIdList.add((int) items[i].nodeId);
                dciIdList.add((int) items[i].dciId);
                int color = items[i].getColorAsInt();
                colorList.add(color == -1 ? Colors.DEFAULT_ITEM_COLORS[i] : swapRGB(color));
                lineWidthList.add(items[i].lineWidth);
                nameList.add(items[i].name);
            }

            // Pass them to activity
            newIntent.putExtra("numGraphs", items.length);
            newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
            newIntent.putIntegerArrayListExtra("dciIdList", dciIdList);
            newIntent.putIntegerArrayListExtra("colorList", colorList);
            newIntent.putIntegerArrayListExtra("lineWidthList", lineWidthList);
            newIntent.putStringArrayListExtra("nameList", nameList);
            newIntent.putExtra("graphTitle", adapter.TrimGroup(gs.getName()));
            if (gs.getTimePeriod().getTimeFrameType() == TimeFrameType.FIXED) {
                newIntent.putExtra("timeFrom", gs.getTimeFrom().getTime());
                newIntent.putExtra("timeTo", gs.getTimeTo().getTime());
            } else {    // Back from now
                newIntent.putExtra("timeFrom", System.currentTimeMillis() - gs.getTimeRangeMillis());
                newIntent.putExtra("timeTo", System.currentTimeMillis());
            }
            startActivity(newIntent);
        }
    }

    /**
     * Swap RGB color (R <--> B)
     */
    private int swapRGB(int color) {
        return ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16); // R | G | B
    }

    /*
     * (non-Javadoc)
     *
     * @see android.app.Activity#onDestroy()
     */
    @Override
    protected void onDestroy() {
        service.registerGraphBrowser(null);
        super.onDestroy();
    }

    /**
     * Refresh graphs list reloading from server
     */
    public void refreshList() {
        new LoadDataTask().execute();
    }

    /**
     * Internal task for loading predefined graphs data
     */
    private class LoadDataTask extends AsyncTask<Object, Void, List<GraphDefinition>> {
        @Override
        protected void onPreExecute() {
            if (dialog != null) {
                dialog.setMessage(getString(R.string.progress_gathering_data));
                dialog.setIndeterminate(true);
                dialog.setCancelable(false);
                dialog.show();
            }
        }

        @Override
        protected List<GraphDefinition> doInBackground(Object... params) {
            List<GraphDefinition> graphs = null;
            try {
                NXCSession session = service.getSession();
                if (session != null)
                    graphs = session.getPredefinedGraphs(false);
            } catch (NXCException e) {
                Log.e(TAG, "NXCException while executing LoadDataTask.doInBackground", e);
                e.printStackTrace();
            } catch (IOException e) {
                Log.e(TAG, "IOException while executing LoadDataTask.doInBackground", e);
                e.printStackTrace();
            }
            return graphs;
        }

        @Override
        protected void onPostExecute(List<GraphDefinition> result) {
            if (dialog != null)
                dialog.cancel();
            if (result != null) {
                adapter.setGraphs(result);
                adapter.notifyDataSetChanged();
            }
        }
    }
}
