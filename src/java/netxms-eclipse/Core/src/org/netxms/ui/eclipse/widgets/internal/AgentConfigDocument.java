package org.netxms.ui.eclipse.widgets.internal;

import org.eclipse.jface.text.Document;
import org.eclipse.jface.text.IDocument;
import org.eclipse.jface.text.rules.EndOfLineRule;
import org.eclipse.jface.text.rules.FastPartitioner;
import org.eclipse.jface.text.rules.IPredicateRule;
import org.eclipse.jface.text.rules.RuleBasedPartitionScanner;
import org.eclipse.jface.text.rules.SingleLineRule;
import org.eclipse.jface.text.rules.Token;

public class AgentConfigDocument extends Document
{
	public static final String CONTENT_COMMENTS = "COMMENTS"; //$NON-NLS-1$
	public static final String CONTENT_STRING = "STRING"; //$NON-NLS-1$
	public static final String CONTENT_SECTION = "SECTION"; //$NON-NLS-1$
   public static final String CONTENT_ENVVAR = "ENVVAR"; //$NON-NLS-1$

   public static final String[] CONTENT_TYPES = { IDocument.DEFAULT_CONTENT_TYPE, CONTENT_COMMENTS, CONTENT_STRING, CONTENT_SECTION, CONTENT_ENVVAR };

   private static final SingleLineRule NEW_FORMAT_SECTION_RULE;
	private static final EndOfLineRule OLD_FORMAT_SECTION_RULE;

	static
	{
      NEW_FORMAT_SECTION_RULE = new SingleLineRule("[", "]", new Token(CONTENT_SECTION), (char)0, true, false); //$NON-NLS-1$ //$NON-NLS-2$
      NEW_FORMAT_SECTION_RULE.setColumnConstraint(0);

      OLD_FORMAT_SECTION_RULE = new EndOfLineRule("*", new Token(CONTENT_SECTION)); //$NON-NLS-1$
		OLD_FORMAT_SECTION_RULE.setColumnConstraint(0);
	}

	// Document partitioning rules
	private static final IPredicateRule[] CONFIG_RULES = {
      new EndOfLineRule("#", new Token(CONTENT_COMMENTS)), //$NON-NLS-1$
		NEW_FORMAT_SECTION_RULE,
      OLD_FORMAT_SECTION_RULE,
      new SingleLineRule("${", "}", new Token(CONTENT_ENVVAR), (char)0, true, false), //$NON-NLS-1$ //$NON-NLS-2$
      new SingleLineRule("\"", "\"", new Token(CONTENT_STRING), '\\', true, false), //$NON-NLS-1$ //$NON-NLS-2$
		new CodePatternRule(new Token(DEFAULT_CONTENT_TYPE))
	};

	/**
	 * Creates agent config document
	 */
	public AgentConfigDocument(String content)
	{
		super(content);

		final RuleBasedPartitionScanner scanner = new RuleBasedPartitionScanner();
		scanner.setPredicateRules(CONFIG_RULES);
		scanner.setDefaultReturnToken(new Token(IDocument.DEFAULT_CONTENT_TYPE));
		final FastPartitioner partitioner = new FastPartitioner(scanner, CONTENT_TYPES);
		setDocumentPartitioner(partitioner);
		partitioner.connect(this);
	}
}
