package org.netxms.tests;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Testing alarm functionality in EPPR action. test generates an alarm, changes few times state and severity, generate timeout alarm
 * terminates the alarm and checks that these functions are working correctly
 * 
 * @throws Exception
 */
public class EppAlarmTest extends AbstractSessionTest
{
   Alarm alarm = null;

   /**
    * Find alarm 
    * 
    * @param session
    * @param key
    * @return alarm based on the event rule key that is expected to generate the alarm
    * @throws Exception
    */
   public Alarm findAlarmByKey(final NXCSession session, String key) throws Exception
   {
      HashMap<Long, Alarm> allAlarms = session.getAlarms();
      for(HashMap.Entry<Long, Alarm> entry : allAlarms.entrySet())
      {
         if (entry.getValue().getKey().equals(key))
         {
            alarm = entry.getValue();
            return alarm;
         }
      }
      return null;
   }

   /**
    * Testing alarm creating functionality
    * 
    * @throws Exception
    */
   public void testCreateAlarm() throws Exception
   {
      final NXCSession session = connect();
      session.syncObjects();
      session.syncEventTemplates();

      final String templateNameEventDown = "TestEventDown";
      final String templateNameEventUp = "TestEventUp";
      final String ruleEventDownComment = "test comment for TestEventDown event";
      final String ruleEventUpComment = "test comment for TestEventUp event";
      final String alarmKey = "Test Key for TestEventDown event";
      final String alarmMessage = "ALARM MESSAGE for event down";

      AbstractObject node = TestHelper.findManagementServer(session);

      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateNameEventDown);
      EventTemplate eventTestTemplate2 = TestHelperForEpp.findOrCreateEvent(session, templateNameEventUp);

      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      // Searching for the alarm generation test rule based on the specified comment; if not found, creates a new one.
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, ruleEventDownComment, eventTestTemplate, node);
      testRule.setAlarmKey(alarmKey);
      testRule.setFlags(testRule.getFlags() | testRule.GENERATE_ALARM);
      testRule.setAlarmMessage(alarmMessage + "%N"); // %N to understand which event alarm is generated
      testRule.setAlarmSeverity(Severity.UNKNOWN);
      testRule.setAlarmTimeout(4);
      List<Long> evnts = new ArrayList<Long>();
      evnts.add(eventTestTemplate.getCode());
      evnts.add(testRule.getAlarmTimeoutEvent());
      testRule.setEvents(evnts);
      eventTestTemplate.setSeverity(Severity.NORMAL); // Changing it back so that the test can be run multiple times in a row
                                                      // without deleting EPPR
      session.modifyEventObject(eventTestTemplate);
      session.saveEventProcessingPolicy(policy);

      // Searches for the alarm termination test rule based on the specified comment; if not found, creates a new one.
      EventProcessingPolicyRule testRule2 = TestHelperForEpp.findOrCreateRule(session, policy, ruleEventUpComment, eventTestTemplate2, node);
      testRule2.setAlarmKey(alarmKey);
      testRule2.setAlarmSeverity(Severity.TERMINATE);
      testRule2.setFlags(testRule2.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
      testRule2.setFlags(testRule2.getFlags() | testRule2.TERMINATE_BY_REGEXP);
      session.saveEventProcessingPolicy(policy);

      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null); // sending event which generated
                                                                                                    // alarm to the server

      alarm = findAlarmByKey(session, alarmKey); // founding alarm
      assertNotNull(alarm);
      Thread.sleep(500);

      // Check that the alarm was generated based on the TestEventDown
      assertEquals(alarm.getMessage(), alarmMessage + eventTestTemplate.getName());
      assertEquals(eventTestTemplate.getSeverity(), alarm.getCurrentSeverity());
      assertEquals(alarm.getState(), Alarm.STATE_OUTSTANDING);

      Thread.sleep(6000);
      alarm = findAlarmByKey(session, alarmKey);

      // Check that the alarm was generated based on the TimeOutEvent
      assertEquals(alarm.getMessage(), alarmMessage + session.getEventName(testRule.getAlarmTimeoutEvent())); // ne rabotajet!!!

      eventTestTemplate.setSeverity(Severity.CRITICAL); // Changing the alarm severity through the event test template
      session.modifyEventObject(eventTestTemplate);
      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null);
      alarm = findAlarmByKey(session, alarmKey);
      assertEquals(eventTestTemplate.getSeverity(), alarm.getCurrentSeverity());// checking that alarm severity is Critical

      testRule.setAlarmSeverity(Severity.RESOLVE); // changing alarm STATE
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null);
      alarm = findAlarmByKey(session, alarmKey);

      assertEquals(alarm.getState(), Alarm.STATE_RESOLVED);// checking that alarm is resolved

      testRule.setAlarmSeverity(Severity.MAJOR); // Changing the severity of testRule to MAJOR
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null);
      alarm = findAlarmByKey(session, alarmKey);

      assertEquals(testRule.getAlarmSeverity(), alarm.getCurrentSeverity());// checking that alarm takes severity from rule

      session.sendEvent(0, templateNameEventUp, node.getObjectId(), new String[] {}, null, null); // sending rule which terminated
                                                                                                  // alarm
      Thread.sleep(500);
      alarm = findAlarmByKey(session, alarmKey);
      assertNull(alarm); // checking that cannot find the alarm in the list of alarms, indicating that it is terminated

      session.closeEventProcessingPolicy();
      session.disconnect();

   }
}
