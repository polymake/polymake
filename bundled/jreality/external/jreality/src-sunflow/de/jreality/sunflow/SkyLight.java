package de.jreality.sunflow;

import org.sunflow.core.ParameterList;
import org.sunflow.image.ChromaticitySpectrum;
import org.sunflow.image.Color;
import org.sunflow.image.ConstantSpectralCurve;
import org.sunflow.image.IrregularSpectralCurve;
import org.sunflow.image.RGBSpace;
import org.sunflow.image.RegularSpectralCurve;
import org.sunflow.image.SpectralCurve;
import org.sunflow.image.XYZColor;
import org.sunflow.math.MathUtils;
import org.sunflow.math.OrthoNormalBasis;
import org.sunflow.math.Vector3;

public class SkyLight  {
	// sunflow parameters
	private OrthoNormalBasis basis;
	private float scalingFactor;
	// parameters to the model
	private Vector3 sunDirWorld;
	private float turbidity;
	// derived quantities
	private Vector3 sunDir;
	// private float sunSolidAngle;
	private SpectralCurve sunSpectralRadiance;
	private Color sunColor;
	private float sunTheta;
	private double zenithY, zenithx, zenithy;
	private final double[] perezY = new double[5];
	private final double[] perezx = new double[5];
	private final double[] perezy = new double[5];
	private int downColor = new java.awt.Color(36,36,36).getRGB();
	private float threshold;
	
	// constant data
	private static final float[] solAmplitudes = { 165.5f, 162.3f, 211.2f,
		258.8f, 258.2f, 242.3f, 267.6f, 296.6f, 305.4f, 300.6f, 306.6f,
		288.3f, 287.1f, 278.2f, 271.0f, 272.3f, 263.6f, 255.0f, 250.6f,
		253.1f, 253.5f, 251.3f, 246.3f, 241.7f, 236.8f, 232.1f, 228.2f,
		223.4f, 219.7f, 215.3f, 211.0f, 207.3f, 202.4f, 198.7f, 194.3f,
		190.7f, 186.3f, 182.6f };
	private static final RegularSpectralCurve solCurve = new RegularSpectralCurve(solAmplitudes, 380, 750);
	private static final float[] k_oWavelengths = { 300, 305, 310, 315, 320,
		325, 330, 335, 340, 345, 350, 355, 445, 450, 455, 460, 465, 470,
		475, 480, 485, 490, 495, 500, 505, 510, 515, 520, 525, 530, 535,
		540, 545, 550, 555, 560, 565, 570, 575, 580, 585, 590, 595, 600,
		605, 610, 620, 630, 640, 650, 660, 670, 680, 690, 700, 710, 720,
		730, 740, 750, 760, 770, 780, 790, };
	private static final float[] k_oAmplitudes = { 10.0f, 4.8f, 2.7f, 1.35f,
		.8f, .380f, .160f, .075f, .04f, .019f, .007f, .0f, .003f, .003f,
		.004f, .006f, .008f, .009f, .012f, .014f, .017f, .021f, .025f,
		.03f, .035f, .04f, .045f, .048f, .057f, .063f, .07f, .075f, .08f,
		.085f, .095f, .103f, .110f, .12f, .122f, .12f, .118f, .115f, .12f,
		.125f, .130f, .12f, .105f, .09f, .079f, .067f, .057f, .048f, .036f,
		.028f, .023f, .018f, .014f, .011f, .010f, .009f, .007f, .004f, .0f,
		.0f };
	private static final float[] k_gWavelengths = { 759, 760, 770, 771 };
	private static final float[] k_gAmplitudes = { 0, 3.0f, 0.210f, 0 };
	private static final float[] k_waWavelengths = { 689, 690, 700, 710, 720,
		730, 740, 750, 760, 770, 780, 790, 800 };
	private static final float[] k_waAmplitudes = { 0f, 0.160e-1f, 0.240e-1f,
		0.125e-1f, 0.100e+1f, 0.870f, 0.610e-1f, 0.100e-2f, 0.100e-4f,
		0.100e-4f, 0.600e-3f, 0.175e-1f, 0.360e-1f };

	private static final IrregularSpectralCurve k_oCurve = new IrregularSpectralCurve(k_oWavelengths, k_oAmplitudes);
	private static final IrregularSpectralCurve k_gCurve = new IrregularSpectralCurve(k_gWavelengths, k_gAmplitudes);
	private static final IrregularSpectralCurve k_waCurve = new IrregularSpectralCurve(k_waWavelengths, k_waAmplitudes);

	public SkyLight() {
		sunDirWorld = new Vector3(1, 0.01f, .5f);
		scalingFactor = 1.0f / 4000;
		turbidity = 6;
		basis = OrthoNormalBasis.makeFromWV(new Vector3(0, 1, 0), new Vector3(1, 0, 0));
		initSunSky();
	}

	private SpectralCurve computeAttenuatedSunlight(float theta, float turbidity) {
		float[] data = new float[91]; // holds the sunsky curve data
		final double alpha = 1.3;
		final double lozone = 0.35;
		final double w = 2.0;
		double beta = 0.04608365822050 * turbidity - 0.04586025928522;
		// Relative optical mass
		double m = 1.0 / (Math.cos(theta) + 0.15 * Math.pow(93.885 - theta / Math.PI * 180.0, -1.253));
		for (int i = 0, lambda = 350; lambda <= 800; i++, lambda += 5) {
			// Rayleigh scattering
			double tauR = Math.exp(-m * 0.008735 * Math.pow((double) lambda / 1000.0, -4.08));
			// Aerosol (water + dust) attenuation
			double tauA = Math.exp(-m * beta * Math.pow((double) lambda / 1000.0, -alpha));
			// Attenuation due to ozone absorption
			double tauO = Math.exp(-m * k_oCurve.sample(lambda) * lozone);
			// Attenuation due to mixed gases absorption
			double tauG = Math.exp(-1.41 * k_gCurve.sample(lambda) * m / Math.pow(1.0 + 118.93 * k_gCurve.sample(lambda) * m, 0.45));
			// Attenuation due to water vapor absorption
			double tauWA = Math.exp(-0.2385 * k_waCurve.sample(lambda) * w * m / Math.pow(1.0 + 20.07 * k_waCurve.sample(lambda) * w * m, 0.45));
			// 100.0 comes from solAmplitudes begin in wrong units.
			double amp = 100.0 * solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA;
			data[i] = (float) amp;
		}
		return new RegularSpectralCurve(data, 350, 800);
	}

	private double perezFunction(final double[] lam, double theta, double gamma, double lvz) {
		double den = ((1.0 + lam[0] * Math.exp(lam[1])) * (1.0 + lam[2] * Math.exp(lam[3] * sunTheta) + lam[4] * Math.cos(sunTheta) * Math.cos(sunTheta)));
		double num = ((1.0 + lam[0] * Math.exp(lam[1] / Math.cos(theta))) * (1.0 + lam[2] * Math.exp(lam[3] * gamma) + lam[4] * Math.cos(gamma) * Math.cos(gamma)));
		return lvz * num / den;
	}

	private void initSunSky() {
		// perform all the required initialization of constants
		sunDirWorld.normalize();
		sunDir = basis.untransform(sunDirWorld, new Vector3());
		sunDir.normalize();
		sunTheta = (float) Math.acos(MathUtils.clamp(sunDir.z, -1, 1));
		if (sunDir.z > 0) {
			sunSpectralRadiance = computeAttenuatedSunlight(sunTheta, turbidity);
			// produce color suitable for rendering
			sunColor = RGBSpace.SRGB.convertXYZtoRGB(sunSpectralRadiance.toXYZ()).constrainRGB();
		} else {
			sunSpectralRadiance = new ConstantSpectralCurve(0);
			sunColor = Color.BLACK;
			System.out.println("Sun is below the horizon!");
		}
		// sunSolidAngle = (float) (0.25 * Math.PI * 1.39 * 1.39 / (150 * 150));
		float theta2 = sunTheta * sunTheta;
		float theta3 = sunTheta * theta2;
		float T = turbidity;
		float T2 = turbidity * turbidity;
		double chi = (4.0 / 9.0 - T / 120.0) * (Math.PI - 2.0 * sunTheta);
		zenithY = (4.0453 * T - 4.9710) * Math.tan(chi) - 0.2155 * T + 2.4192;
		zenithY *= 1000; /* conversion from kcd/m^2 to cd/m^2 */
		zenithx = (0.00165 * theta3 - 0.00374 * theta2 + 0.00208 * sunTheta + 0) * T2 + (-0.02902 * theta3 + 0.06377 * theta2 - 0.03202 * sunTheta + 0.00394) * T + (0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * sunTheta + 0.25885);
		zenithy = (0.00275 * theta3 - 0.00610 * theta2 + 0.00316 * sunTheta + 0) * T2 + (-0.04212 * theta3 + 0.08970 * theta2 - 0.04153 * sunTheta + 0.00515) * T + (0.15346 * theta3 - 0.26756 * theta2 + 0.06669 * sunTheta + 0.26688);

		perezY[0] = 0.17872 * T - 1.46303;
		perezY[1] = -0.35540 * T + 0.42749;
		perezY[2] = -0.02266 * T + 5.32505;
		perezY[3] = 0.12064 * T - 2.57705;
		perezY[4] = -0.06696 * T + 0.37027;

		perezx[0] = -0.01925 * T - 0.25922;
		perezx[1] = -0.06651 * T + 0.00081;
		perezx[2] = -0.00041 * T + 0.21247;
		perezx[3] = -0.06409 * T - 0.89887;
		perezx[4] = -0.00325 * T + 0.04517;

		perezy[0] = -0.01669 * T - 0.26078;
		perezy[1] = -0.09495 * T + 0.00921;
		perezy[2] = -0.00792 * T + 0.21023;
		perezy[3] = -0.04405 * T - 1.65369;
		perezy[4] = -0.01092 * T + 0.05291;
	}

	public boolean update(ParameterList pl) {
		Vector3 up = pl.getVector("up", null);
		Vector3 east = pl.getVector("east", null);
		if (up != null && east != null)
			basis = OrthoNormalBasis.makeFromWV(up, east);
		else if (up != null)
			basis = OrthoNormalBasis.makeFromW(up);
		sunDirWorld = pl.getVector("sundir", sunDirWorld);
		turbidity = pl.getFloat("turbidity", turbidity);
		// recompute model
		initSunSky();
		return true;
	}

	private Color getSkyRGB(Vector3 dir) {
		dir = basis.untransform(dir, new Vector3());
		if (dir.z < -threshold)
			return Color.BLACK ;
		if (dir.z < 0.001f)
			dir.z = 0.001f;
		dir.normalize();
		double theta = Math.acos(MathUtils.clamp(dir.z, -1, 1));
		double gamma = Math.acos(MathUtils.clamp(Vector3.dot(dir, sunDir), -1, 1));
		double x = perezFunction(perezx, theta, gamma, zenithx);
		double y = perezFunction(perezy, theta, gamma, zenithy);
		double Y = perezFunction(perezY, theta, gamma, zenithY);
		XYZColor c = new ChromaticitySpectrum((float) x, (float) y).toXYZ();
		float X = (float) (c.getX() * Y / c.getY());
		float Z = (float) (c.getZ() * Y / c.getY());
		return RGBSpace.SRGB.convertXYZtoRGB(X, (float) Y, Z);
	}

	public int getRadiance(double dirX, double dirY, double dirZ) {
		//if (dirY <0) return downColor;
		Color c = getSkyRGB(new Vector3((float)dirX, (float)dirY, (float)dirZ));
		c.mul(scalingFactor/4); // simple tone mapping
		return c.constrainRGB().toNonLinear().toRGB();
	}

	public float getThreshold() {
		return threshold;
	}

	public void setThreshold(float threshold) {
		this.threshold = threshold;
	}

	public int getDownColor() {
		return downColor;
	}

	public void setDownColor(int downColor) {
		this.downColor = downColor;
	}

	public Color getSunColor() {
		return sunColor;
	}
}