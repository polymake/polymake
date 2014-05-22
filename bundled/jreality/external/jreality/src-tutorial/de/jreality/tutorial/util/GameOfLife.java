package de.jreality.tutorial.util;

import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.image.BufferedImage;

import de.jreality.shader.ImageData;

public class GameOfLife {

	int height = 128;
	int width =  128;
	int current = 0;
	double[][] lifeColors = {{0,.6,1,1}, {1, 1,1,0}}; 
	byte[] faceColors;
	int[][][] lifeBoard;
	BufferedImage bufferedImage;
	Graphics2D g;
	public GameOfLife(BufferedImage bi)	{
		this.width = bi.getWidth();
		this.height = bi.getHeight();
		bufferedImage = bi;
		g = bufferedImage.createGraphics();
		lifeBoard = new int[2][height][width];
		resetBoard();
		faceColors = new byte[width*height*4];		
		updateColors();
	}
	public void resetBoard() {
		for (int i = 0; i<height; ++i)	{
			for (int j = 0; j<width; ++j)	{
				lifeBoard[current][i][j] = (Math.random() > .7) ? 1 : 0;
			}
		}
	}
	public void updateLife()	{
		for (int i = 0; i<height; ++i)	{
			for (int j = 0; j<width; ++j)	{
				int curval = lifeBoard[current][i][j] % 2;
				int sum = 0;
				for (int n = -1; n<2; ++n)	{
					for (int m = -1; m<2; ++m)	{
						sum += lifeBoard[current][(i+n+height)%height][(j+m+width)%width];
					}
				}
				if ( (curval == 1 && (sum == 3 || sum == 4)) ||
					 (curval == 0) && (sum == 3))
					lifeBoard[1-current][i][j] = 1;
				else lifeBoard[1-current][i][j] = 0;
			}
		}
		current = 1 - current;
	}
	public void updateColors()	{
		for (int i = 0; i<height; ++i)	{
			for (int j = 0; j<width; ++j)	{
				for (int k = 0; k<4; ++k)	{
					faceColors[4*(i*width +j)+k] = (byte) (255.0 * lifeColors[ lifeBoard[current][i][j]][k]);
				}
			}
		}
	}
	public Image currentValue()	{
		return currentValue.getImage();
	}
	ImageData currentValue;
	public void update()	{
		updateLife();
		updateColors();
		currentValue = new ImageData(faceColors, width, height);
//		Image im = currentValue.getImage();
//		g.drawImage(im, 0, 0, null);
	}
}
