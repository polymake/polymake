package de.jreality.reader.mathematica;

import java.awt.Color;

public class MHelper {
	public static Color cmykaToRgba(double c,double m,double y,double k,double a){
		float r,g,b,alph;
		r=(float) ((1-c)*(1-k));
		g=(float) ((1-m)*(1-k));
		b=(float) ((1-y)*(1-k));
		alph=(float) (1-a);
		return new Color(r,g,b,alph);
	}
	public static Color greyLevelToRgba(double gr,double a){
		return new Color((float)gr,(float)gr,(float)gr,(float)a);
	}
	public static Color hsbaToRgba(double h,double s,double b,double a){
		Color col;
		col= Color.getHSBColor((float)h, (float)s,(float) b);
		if(a<0) return col;
		return new Color((float)col.getRed()/255,(float)col.getGreen()/255,(float)col.getBlue()/255,(float)a);
	}
	public static Color rgbaToRgba(double r,double g,double b,double a){
		return new Color((float)r,(float)g,(float)b,(float)a);
	}
	public static Color rgbaToRgba(Color c,double a){
		return new Color((float)(c.getRed()/256.0),(float)(c.getGreen()/256.0),(float)(c.getBlue()/256.0),(float)a);
	}
	public static Color colorToRgba(double[] n){
		if(n.length==1) return greyLevelToRgba(n[0], 1);
		if(n.length==2) return greyLevelToRgba(n[0], n[1]);
		if(n.length==3) return rgbaToRgba(n[0], n[1], n[2],1);
		return rgbaToRgba(n[0], n[1], n[2],n[3]);
	}
	
	static double[] getRgbaColor(Color c){
		// retuns a array that represents a color (needed for color-Arrays as double[][])
			double[] col= new double[4];
			col[0]=c.getRed()/256.0;
			col[1]=c.getGreen()/256.0;
			col[2]=c.getBlue()/256.0;
			col[3]=c.getAlpha()/256.0;
			return col ;
		}
		
	public static Color lighter(Color c,double frac){
		double r= c.getRed()/256;
		double g= c.getGreen()/256;
		double b= c.getBlue()/256;
		double a= c.getAlpha()/256;
		r+=(1-r)*frac;
		g+=(1-g)*frac;
		b+=(1-b)*frac;
		return new Color((float)r,(float)g,(float)b,(float)a);
	}
	public static Color darker(Color c,double frac){
		double r= c.getRed()*(1-frac)/256;
		double g= c.getGreen()*(1-frac)/256;
		double b= c.getBlue()*(1-frac)/256;
		double a= c.getAlpha()*(1-frac)/256;
		return new Color((float)r,(float)g,(float)b,(float)a);
	}
	public static Color opacity(Color c,double frac){
		if(c==null)	return	new Color(1,1,1,(float)(frac));
		double r= c.getRed()/256.;
		double g= c.getGreen()/256.;
		double b= c.getBlue()/256.;
		double a= frac;
		return new Color((float)r,(float)g,(float)b,(float)a);
	}
	public static Color spec(Color c,double frac){
		// default ist s=0;
		//TODO
		return c;
	}

}
