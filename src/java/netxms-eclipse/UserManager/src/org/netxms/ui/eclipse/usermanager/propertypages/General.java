/**
 * 
 */
package org.netxms.ui.eclipse.usermanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class General extends PropertyPage
{
	private Text textName;
	private Text textFullName;
	private Text textDescription;
	private String initialName;
	private String initialFullName;
	private String initialDescription;
	private NXCUserDBObject object;
	private NXCSession session;
	
	/**
	 * Default constructor
	 */
	public General()
	{
		super();
		session = NXMCSharedData.getInstance().getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NXCUserDBObject)getElement().getAdapter(NXCUserDBObject.class);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);
      
      // Object ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Object ID",
                                     Long.toString(object.getId()), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
		// Object name
      initialName = new String(object.getName());
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Login name",
      		                                    initialName, WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		// Full name
      if (object instanceof NXCUser)
      {
	      initialFullName = new String(((NXCUser)object).getFullName());
	      textFullName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Full name",
	      		                                        initialFullName, WidgetHelper.DEFAULT_LAYOUT_DATA);
      }
      else
      {
      	initialFullName = "";
      }
      
		// Description
      initialDescription = new String(object.getDescription());
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
                                                       "Description", initialDescription, WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		return dialogArea;
	}
	
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		final String newName = new String(textName.getText());
		final String newDescription = new String(textDescription.getText());
		final String newFullName = (object instanceof NXCUser) ? new String(textFullName.getText()) : "";
		
		if (newName.equals(initialName) && 
		    newDescription.equals(initialDescription) &&
		    newFullName.equals(initialFullName))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		new Job("Update user database object") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					initialName = newName;
					initialFullName = newFullName;
					initialDescription = newDescription;
					
					int fields = NXCSession.USER_MODIFY_LOGIN_NAME | NXCSession.USER_MODIFY_DESCRIPTION;
					object.setName(newName);
					object.setDescription(newDescription);
					if (object instanceof NXCUser)
					{
						((NXCUser)object).setFullName(newFullName);
						fields |= NXCSession.USER_MODIFY_FULL_NAME;
					}
					session.modifyUserDBObject(object, fields);
					
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot change object name: " + e.getMessage(), e);
				}

				if (isApply)
				{
					new UIJob("Update \"General\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							General.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}

				return status;
			}
		}.schedule();
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}


	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
