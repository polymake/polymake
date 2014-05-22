package de.jreality.shader;

import de.jreality.scene.data.AttributeCollection;

public interface HapticShader extends AttributeCollection {

	static final double STIFFNESS_DEFAULT = CommonAttributes.HAPTIC_STIFFNESS_DEFAULT;
	static final double DYNAMIC_FRICTION_DEFAULT = CommonAttributes.HAPTIC_DYNAMIC_FRICTION_DEFAULT;
	static final double STATIC_FRICTION_DEFAULT = CommonAttributes.HAPTIC_STATIC_FRICTION_DEFAULT;
	static final double DAMPING_DEFAULT = CommonAttributes.HAPTIC_DAMPING_DEFAULT;
	
	static final boolean HAPTIC_ENABLED_DEFAULT = CommonAttributes.HAPTIC_ENABLED_DEFAULT;
	
	static final boolean HAPTIC_TOUCHABLE_BACK_DEFAULT = CommonAttributes.HAPTIC_TOUCHABLE_BACK_DEFAULT;
	static final boolean HAPTIC_TOUCHABLE_FRONT_DEFAULT = CommonAttributes.HAPTIC_TOUCHABLE_FRONT_DEFAULT;
	
	public boolean getHapticEnabled();
	public void setHapticEnabled(boolean enabled);
	
	
	public boolean getHapticTouchableBack();
	public void setHapticTouchableBack(boolean enabled);
	
	public boolean getHapticTouchableFront();
	public void setHapticTouchableFront(boolean enabled);

	
	public double getStiffness();
	public void setStiffness(double stiffness);

	public double getDynamicFriction();
	public void setDynamicFriction(double dynamicFriction);

	public double getStaticFriction();
	public void setStaticFriction(double staticFriction);

	public double getDamping();
	public void setDamping(double damping);
}
