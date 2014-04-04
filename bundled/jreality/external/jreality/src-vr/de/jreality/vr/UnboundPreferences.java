package de.jreality.vr;

import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.util.HashMap;
import java.util.prefs.AbstractPreferences;
import java.util.prefs.BackingStoreException;
import java.util.prefs.InvalidPreferencesFormatException;
import java.util.prefs.Preferences;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

class UnboundPreferences extends AbstractPreferences {

	private HashMap<String, String> root;
	private HashMap<String, UnboundPreferences> children;
	private boolean isRemoved = false;
	private UnboundPreferences rootPrefs;


	public static UnboundPreferences createRoot() {
		return new UnboundPreferences(null, "");
	}

	UnboundPreferences( UnboundPreferences prefs, String name ) {
		super( prefs, name );
		if (prefs == null) rootPrefs=this;
		else rootPrefs=prefs.getRoot();
		root = new HashMap<String, String>();
		children = new HashMap<String, UnboundPreferences>();
		try {
			sync();
		}
		catch ( Exception e ) {
			e.printStackTrace();
		}
	}

	@Override
	protected void putSpi( String key, String value ) {
		root.put( key, value );
	}

	@Override
	protected String getSpi( String key ) {
		return ( String ) root.get( key );
	}

	@Override
	protected void removeSpi( String key ) {
		root.remove( key );
	}

	@Override
	protected void removeNodeSpi() throws BackingStoreException {
		isRemoved = true;
	}

	@Override
	protected String[] keysSpi() throws BackingStoreException {
		return ( String[] ) root.keySet().toArray( new String[] {} );
	}

	@Override
	protected String[] childrenNamesSpi() throws BackingStoreException {
		return ( String[] ) children.keySet().toArray( new String[] {} );
	}

	@Override
	protected AbstractPreferences childSpi( String name ) {
		UnboundPreferences child = ( UnboundPreferences ) children.get( name );
		if ( child == null || child.isRemoved() ) {
			try {
				child = new UnboundPreferences( this, name);
				children.put( name, child );
			}
			catch ( Exception e ) {
				e.printStackTrace();
				child = null;
			}
		}
		return child;
	}

	@Override
	protected void syncSpi() throws BackingStoreException {
		// no-op
	}

	protected boolean isRemoved() {
		return isRemoved;
	}

	@Override
	protected void flushSpi() throws BackingStoreException {
		// no-op
	}

	@Override
	public boolean isUserNode() {
		return true;
	}

	private UnboundPreferences getRoot() {
		return rootPrefs;
	}

	
	/******* XML export ***********/
	
	public void localImportPreferences(InputStream is) throws IOException, InvalidPreferencesFormatException {
		try {
			Document doc = loadPrefsDoc(is);
			Element xmlRoot = (Element) doc.getDocumentElement().
			getChildNodes().item(0);
			localImport(getRoot(), xmlRoot);
		} catch(SAXException e) {
			throw new InvalidPreferencesFormatException(e);
		}
	}

	private static Document loadPrefsDoc(InputStream in) throws SAXException, IOException {
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		dbf.setIgnoringElementContentWhitespace(true);
		dbf.setValidating(true);
		dbf.setCoalescing(true);
		dbf.setIgnoringComments(true);
		try {
			class Resolver implements EntityResolver {
				public InputSource resolveEntity(String pid, String sid)
				throws SAXException
				{
					if (sid.equals("http://java.sun.com/dtd/preferences.dtd")) {
						InputSource is;
						is = new InputSource(new StringReader(("<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +

								"<!-- DTD for preferences -->"               +

								"<!ELEMENT preferences (root) >"             +
								"<!ATTLIST preferences"                      +
								" EXTERNAL_XML_VERSION CDATA \"0.0\"  >"     +

								"<!ELEMENT root (map, node*) >"              +
								"<!ATTLIST root"                             +
								"          type (system|user) #REQUIRED >"   +

								"<!ELEMENT node (map, node*) >"              +
								"<!ATTLIST node"                             +
								"          name CDATA #REQUIRED >"           +

								"<!ELEMENT map (entry*) >"                   +
								"<!ATTLIST map"                              +
								"  MAP_XML_VERSION CDATA \"0.0\"  >"         +
								"<!ELEMENT entry EMPTY >"                    +
								"<!ATTLIST entry"                            +
								"          key CDATA #REQUIRED"              +
						"          value CDATA #REQUIRED >")));
						is.setSystemId("http://java.sun.com/dtd/preferences.dtd");
						return is;
					}
					throw new SAXException("Invalid system identifier: " + sid);
				}
			}

			DocumentBuilder db = dbf.newDocumentBuilder();
			db.setEntityResolver(new Resolver());
			return db.parse(new InputSource(in));
		} catch (ParserConfigurationException e) {
			throw new AssertionError(e);
		}
	}

	private static void localImport(Preferences p, Element node) {
		
		NodeList children = node.getChildNodes();
		
		// import prefs of this node
		NodeList entries = children.item(0).getChildNodes();
        for (int i=0, numEntries = entries.getLength(); i < numEntries; i++) {
            Element entry = (Element) entries.item(i);
            p.put(entry.getAttribute("key"), entry.getAttribute("value"));
        }

        // import child nodes
		for (int i=1; i < children.getLength(); i++) {
			Element child = (Element) children.item(i);
			Preferences cp = p.node(child.getAttribute("name"));
			localImport(cp, (Element)children.item(i));
		}
	}


}



