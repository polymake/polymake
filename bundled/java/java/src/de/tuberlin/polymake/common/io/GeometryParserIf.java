/**
 * 
 */
package de.tuberlin.polymake.common.io;

import java.io.BufferedReader;
import java.io.IOException;

import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;

/**
 * @author thilosch
 *
 */
public interface GeometryParserIf {

	public EmbeddedGeometries parse(BufferedReader in) throws IOException;
}
