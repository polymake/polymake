package de.jreality.util;


import java.awt.Color;

public class ColorGradient {

  Color[] color;

  double time = 0;

  public ColorGradient(Color... key) {

    Color[][] range = new Color[key.length - 1][];

    int totalNumberOfColors = 1;

    for (int k = 0; k < range.length; k++) {

      int redAt0   = key[k    ].getRed();
      int greenAt0 = key[k    ].getGreen();
      int blueAt0  = key[k    ].getBlue();
      int redAt1   = key[k +1 ].getRed();
      int greenAt1 = key[k +1 ].getGreen();
      int blueAt1  = key[k +1 ].getBlue();

      int distRed   = Math.abs(redAt0   - redAt1   );
      int distGreen = Math.abs(greenAt0 - greenAt1 );
      int distBlue  = Math.abs(blueAt0  - blueAt1  );

      int maxDist = Math.max(distRed, Math.max(distGreen, distBlue));

      totalNumberOfColors += maxDist;

      float denom = (float)maxDist;

      range[k] = new Color[maxDist+1];

      range[k][0      ] = key[k  ];
      range[k][maxDist] = key[k+1];

      for (int i = 1; i < maxDist; i++) {
        float t = i / denom;

        range[k][i] = new Color(( (1 - t) * redAt0   + t * redAt1  ) / 255,
                                ( (1 - t) * greenAt0 + t * greenAt1) / 255,
                                ( (1 - t) * blueAt0  + t * blueAt1 ) / 255);
        }
    }

    color = new Color[ totalNumberOfColors ];

    for (int k = 0, o=0; k < range.length; o+=range[k].length-1, k++) {
      System.arraycopy(range[k], 0, color, o, range[k].length-1);
    }

    color[totalNumberOfColors-1] = key[key.length-1];
  }

  public ColorGradient() {
    this( new Color[] { Color.blue, Color.cyan, Color.green, Color.yellow, Color.red } );
  }

  public static ColorGradient getFullPeriodicColorGradient() {
    return  new ColorGradient( new Color[] { Color.blue, Color.cyan, Color.green,
                               Color.yellow, Color.red, Color.magenta, Color.blue } );
  }

  public double getTime() {
    return time;
  }

  public void setTime(double aDouble) {
    time = aDouble;
  }

  public Color getColorValue() {
    return getColor(time);
  }

  public Color getColor(double t) {
    if (t < 0) {
      t = 0;
    }

    if (t > 1) {
      t = 1;
    }

    return color[ (int) ( (color.length - 1) * t + 0.5)];
  }

  public Color[] getColor() {
    return color;
  }

}