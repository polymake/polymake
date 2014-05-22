package de.jreality.scene.pick;

public interface HitFilter {

	public boolean accept(double[] from, double[] to, PickResult h);
}
