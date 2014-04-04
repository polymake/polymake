package de.jreality.io.jrs;

import com.thoughtworks.xstream.converters.Converter;
import com.thoughtworks.xstream.mapper.Mapper;

public abstract class AbstractConverter implements Converter {

	  protected Mapper mapper;
	  protected double version;
	  
	  public AbstractConverter(Mapper mapper, double version) {
	    this.mapper = mapper;
	    this.version = version;
	  }


}
