package de.jreality.shader;

import java.beans.IntrospectionException;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;

import de.jtem.beans.DoubleSpinnerEditor;

public class DefaultPolygonShaderBeanInfo extends SimpleBeanInfo {

	public static class CoefficientEditor extends DoubleSpinnerEditor {
		@Override
		protected void customize() {
			model.setMinimum(new Double(0));
			model.setMaximum(new Double(1.));
			model.setStepSize(new Double(.1));
		}
	}
	
	public static class SpecularExponentEditor extends DoubleSpinnerEditor {
		@Override
		protected void customize() {
			model.setMinimum(new Double(0));
			model.setStepSize(new Double(1.));
			model.setValue(CommonAttributes.SPECULAR_COEFFICIENT_DEFAULT);
		}
	}

	public static class AmbientCoefficientEditor extends CoefficientEditor {
		@Override
		protected void customize() {
			super.customize();
			model.setValue(CommonAttributes.AMBIENT_COEFFICIENT_DEFAULT);
		}
	}

	public static class DiffuseCoefficientEditor extends CoefficientEditor {
		@Override
		protected void customize() {
			super.customize();
			model.setValue(CommonAttributes.DIFFUSE_COEFFICIENT_DEFAULT);
		}
	}

	public static class SpecularCoefficientEditor extends CoefficientEditor {
		@Override
		protected void customize() {
			super.customize();
			model.setValue(CommonAttributes.SPECULAR_COEFFICIENT_DEFAULT);
		}
	}
	
	public static class TransparencyEditor extends CoefficientEditor {
		@Override
		protected void customize() {
			super.customize();
			model.setValue(CommonAttributes.TRANSPARENCY_DEFAULT);
		}
	}

	public PropertyDescriptor[] getPropertyDescriptors() {
		Class beanClass = DefaultPolygonShader.class;
		try {  
			PropertyDescriptor ambientCoefficient =
				new PropertyDescriptor("ambientCoefficient", beanClass);
			ambientCoefficient.setPropertyEditorClass(AmbientCoefficientEditor.class);

			PropertyDescriptor diffuseCoefficient =
				new PropertyDescriptor("diffuseCoefficient", beanClass);
			diffuseCoefficient.setPropertyEditorClass(DiffuseCoefficientEditor.class);

			PropertyDescriptor specularCoefficient =
				new PropertyDescriptor("specularCoefficient", beanClass);
			specularCoefficient.setPropertyEditorClass(SpecularCoefficientEditor.class);
			
			PropertyDescriptor transparency =
				new PropertyDescriptor("transparency", beanClass);
			transparency.setPropertyEditorClass(TransparencyEditor.class);

			PropertyDescriptor smoothShading =
				new PropertyDescriptor("smoothShading", beanClass);

			PropertyDescriptor ambientColor =
				new PropertyDescriptor("ambientColor", beanClass);
			
			PropertyDescriptor diffuseColor =
				new PropertyDescriptor("diffuseColor", beanClass);

			PropertyDescriptor specularColor =
				new PropertyDescriptor("specularColor", beanClass);

			PropertyDescriptor specularExponent =
				new PropertyDescriptor("specularExponent", beanClass);
			specularExponent.setPropertyEditorClass(SpecularExponentEditor.class);

			PropertyDescriptor rv[] = {
					ambientCoefficient,
					ambientColor,
					diffuseCoefficient,
					diffuseColor,
					specularCoefficient,
					specularExponent,
					specularColor,
					transparency,
					smoothShading
			};
			return rv;
		} catch (IntrospectionException e) {
			throw new Error(e.toString());
		}
	}
}
