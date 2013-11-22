package org.netxms.ui.eclipse.objectmanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objectmanager.messages"; //$NON-NLS-1$
   public static String AbstractNodePoll_Error;
   public static String AbstractNodePoll_ErrorText;
   public static String AddClusterNode_JobError;
   public static String AddClusterNode_JobTitle;
   public static String AutoApply_AutoApply;
   public static String AutoApply_AutoRemove;
   public static String AutoApply_JobError;
   public static String AutoApply_JobName;
   public static String AutoApply_Script;
   public static String AutoBind_AutoBind;
   public static String AutoBind_AUtoUnbind;
   public static String AutoBind_JobError;
   public static String AutoBind_JobName;
   public static String AutoBind_Script;
   public static String BindObject_JobError;
   public static String BindObject_JobTitle;
   public static String ChangeInterfaceExpectedState_JobError;
   public static String ChangeInterfaceExpectedState_JobTitle;
   public static String ChangeZone_JobError;
   public static String ChangeZone_JobTitle;
   public static String ClusterNetworks_Add;
   public static String ClusterNetworks_ColAddress;
   public static String ClusterNetworks_ColMask;
   public static String ClusterNetworks_Delete;
   public static String ClusterNetworks_JobError;
   public static String ClusterNetworks_JobName;
   public static String ClusterNetworks_Modify;
   public static String ClusterResources_Add;
   public static String ClusterResources_ColName;
   public static String ClusterResources_ColVIP;
   public static String ClusterResources_Delete;
   public static String ClusterResources_JobError;
   public static String ClusterResources_JobName;
   public static String ClusterResources_Modify;
   public static String ConditionScript_JobError;
   public static String ConditionScript_JobName;
   public static String ConditionScript_Script;
   public static String CreateCluster_Cluster;
   public static String CreateCluster_JobError;
   public static String CreateCluster_JobTitle;
   public static String CreateCondition_Condition;
   public static String CreateCondition_JobError;
   public static String CreateCondition_JobTitle;
   public static String CreateContainer_Container;
   public static String CreateContainer_JobError;
   public static String CreateContainer_JobTitle;
   public static String CreateInterface_JobError;
   public static String CreateInterface_JobTitle;
   public static String CreateMobileDevice_JobError;
   public static String CreateMobileDevice_JobTitle;
   public static String CreateNetworkService_JobError;
   public static String CreateNetworkService_JobTitle;
   public static String CreateNode_JobError;
   public static String CreateNode_JobTitle;
   public static String CreateRack_JobError;
   public static String CreateRack_JobTitle;
   public static String CreateRack_Rack;
   public static String CreateZone_JobError;
   public static String CreateZone_JobTitle;
   public static String DeleteObject_ConfirmDelete;
   public static String DeleteObject_ConfirmQuestionPlural;
   public static String DeleteObject_ConfirmQuestionSingular;
   public static String DeleteObject_JobDescription;
   public static String DeleteObject_JobError;
   public static String Manage_JobDescription;
   public static String Manage_JobError;
   public static String MapAppearance_Image;
   public static String MapAppearance_JobError;
   public static String MapAppearance_JobName;
   public static String MapAppearance_Submap;
   public static String RemoveClusterNode_JobError;
   public static String RemoveClusterNode_JobTitle;
   public static String UnbindObject_JobError;
   public static String UnbindObject_JobTitle;
   public static String Unmanage_JobDescription;
   public static String Unmanage_JobError;
   public static String ZoneSelectionDialog_EmptySelectionWarning;
   public static String ZoneSelectionDialog_Title;
   public static String ZoneSelectionDialog_Warning;
   public static String ZoneSelectionDialog_ZoneObject;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
	{
 }


	private static Messages instance = new Messages();

	public static Messages get()
	{
		return instance;
	}

}
