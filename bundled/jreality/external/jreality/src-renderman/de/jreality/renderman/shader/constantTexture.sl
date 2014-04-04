surface
constantTexture (   string texturename = "";
					float tm[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
					float transparencyenabled = 1;)
{
        color Ct;
        float tr;
        if (texturename != "")    {
        	matrix textureMatrix = matrix "current" (tm[0],tm[1],tm[2],tm[3],tm[4],tm[5],tm[6],tm[7],tm[8],tm[9],tm[10],tm[11],tm[12],tm[13],tm[14],tm[15]);
        	point a = point (s,t,0);
			point p = transform( textureMatrix , a);
			float ss = xcomp(p);
			float tt = ycomp(p);
			tr = float texture(texturename[3],ss,tt, "fill",1);        
	        //tr = float texture(texturename[3], "fill",1);
            Ct = color texture (texturename);
            Ci = Ct * Cs;
            if (transparencyenabled != 0.0)
  	    		Oi = tr*Os;
    		else {
        		if (tr==0)
            		Oi=0;
       			else
            		Oi=1;   
    		}
        }
        else {
            Ci = Cs;
            Oi = Os;
        }
}