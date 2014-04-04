package de.jreality.shader;

import java.beans.IntrospectionException;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jtem.beans.ChoiceEditor;
import de.jtem.beans.DoubleEditor;

public class Texture2DBeanInfo extends SimpleBeanInfo {
	
	public static class ApplyModeEditor extends ChoiceEditor {
		protected void defineValuesToStrings() {
			valuesToStrings.put(new Integer(Texture2D.GL_MODULATE), "modulate");
			valuesToStrings.put(new Integer(Texture2D.GL_COMBINE), "combine");
			valuesToStrings.put(new Integer(Texture2D.GL_REPLACE), "replace");
			valuesToStrings.put(new Integer(Texture2D.GL_DECAL), "decal");
		}
		@Override
		protected String getNameOfNull() {
			return "inherit";
		}
	}
	
	public static class RepeatModeEditor extends ChoiceEditor {
		protected void defineValuesToStrings() {
			valuesToStrings.put(new Integer(Texture2D.GL_REPEAT), "repeat");
			valuesToStrings.put(new Integer(Texture2D.GL_CLAMP), "clamp");
			valuesToStrings.put(new Integer(Texture2D.GL_CLAMP_TO_EDGE), "clamp to edge");
		}
		@Override
		protected String getNameOfNull() {
			return "inherit";
		}
	}
	
	public static class TextureMatrixEditor extends DoubleEditor {
		private Matrix unit = MatrixBuilder.euclidean().getMatrix();
		private double d = 1.0;
		public TextureMatrixEditor() {
			super();
			setValue(unit);
		}
		@Override
		public String getAsText() {
//			if (getValue() == null) return "";
//			else 
			return String.valueOf(d);
		}
		@Override
		public void setAsText(String text) {
			if (text == null) setValue(unit);  //null
			else {
				try {
					d = Double.parseDouble(text);
					setValue(MatrixBuilder.euclidean().scale(d).getMatrix());
				} catch (NumberFormatException nfe) {
					setValue(unit);  //null
				}
			}
		}
	}
	
//	 GL_MIRRORED_REPEAT =  0x8370;
//	  public static final int GL_REPEAT = 0x2901;
//	  public static final int GL_CLAMP = 0x2900;
//	  public static final int GL_CLAMP_TO_EDGE = 0x812F;
//	  public static final int CLAMP = 0;
//	  public static final int REPEAT = 1;
	
	public PropertyDescriptor[] getPropertyDescriptors() {
		Class beanClass = Texture2D.class;
		try {  
			PropertyDescriptor applyMode =
				new PropertyDescriptor("applyMode", beanClass);
			applyMode.setPropertyEditorClass(
					ApplyModeEditor.class
			);
			PropertyDescriptor image =
				new PropertyDescriptor("image", beanClass);
			PropertyDescriptor magFilter =
				new PropertyDescriptor("magFilter", beanClass);
			PropertyDescriptor minFilter =
				new PropertyDescriptor("minFilter", beanClass);
			PropertyDescriptor blendColor =
				new PropertyDescriptor("blendColor", beanClass);
			PropertyDescriptor combineMode =
				new PropertyDescriptor("combineMode", beanClass);
			
			PropertyDescriptor textureMatrix =
				new PropertyDescriptor("textureMatrix", beanClass);
			textureMatrix.setPropertyEditorClass(
					TextureMatrixEditor.class
			);
			textureMatrix.setDisplayName("scale");
			
			PropertyDescriptor repeatS =
				new PropertyDescriptor("repeatS", beanClass);
			repeatS.setPropertyEditorClass(
					RepeatModeEditor.class
			);
			PropertyDescriptor repeatT =
				new PropertyDescriptor("repeatT", beanClass);
			repeatT.setPropertyEditorClass(
					RepeatModeEditor.class
			);
			
			PropertyDescriptor rv[] = {
					minFilter,
					magFilter,
					image,
					blendColor,
					textureMatrix,
					repeatS,
					repeatT,
					applyMode,
					combineMode
			};
			return rv;
		} catch (IntrospectionException e) {
			throw new Error(e.toString());
		}
	}
}
