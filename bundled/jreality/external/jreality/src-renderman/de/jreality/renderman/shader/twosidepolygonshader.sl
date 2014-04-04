/* defaultpolygonshader.sl - like standard paintedplastic 
 * PARAMETERS:
 *    Ka, Kd, Ks, roughness, specularcolor - the usual meaning.
 *    Kr: strength of reflection map
 *    texturename - the name of the texture file.
 * TODO:
 *    compare this to the corresponding OpenGL shader to make sure that
 *    all the bells and whistles of the latter are implemented here --
 *    mainly apply mode, combine mode, blend color, etc.
 * Author: Bleicher
 */


// replace the standard specular function with one that matches the other jreality backends
float 
myspecularbrdf(vector L, N, V; float roughness)
{
    vector H = normalize(L+V);
    return pow(max(0, N.H), 1/roughness);
}

color myspecular( normal N; vector V; float roughness )
{
    color C = 0;
    illuminance( P, N, PI/2 ) 
    C += Cl * myspecularbrdf(normalize(L), N, V, roughness);
    return C;
}

surface
twosidepolygonshader ( 
    float Kafront = 0, 
	Kdfront = .5, 
	Ksfront = .5, 
	Krfront = .5, 
	roughnessfront = .1, 
	reflectionBlendfront = .6;    
	color diffusecolorfront = 1;
	color specularcolorfront = 1;
    float lightingfront = 1;   
    float transparencyenabledfront = 1;
    float tmfront[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	string texturenamefront = ""; 
	string reflectionmapfront = ""; 
    
    float Kaback = 0, 
	Kdback = .5, 
	Ksback = .5, 
	Krback = .5, 
	roughnessback = .1, 
	reflectionBlendback = .6;
	color diffusecolorback = 1;
	color specularcolorback = 1;
    float lightingback = 1;
    float transparencyenabledback = 1; 
    float tmback[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	string texturenameback = ""; 
	string reflectionmapback = ""; 

    float raytracedreflections = 0;
    float raytracedvolumes = 0;
    
	)
{
  normal Nf;
  vector V, D;
  color Ct;
  float tr = 1;
  float debug = 0;
  
  
  float Ka = Kafront; 
  float Kd = Kdfront; 
  float Ks = Ksfront; 
  float Kr = Krfront; 
  float roughness = roughnessfront; 
  float reflectionBlend = reflectionBlendfront;
  color diffusecolor = diffusecolorfront;
  color specularcolor = specularcolorfront; 
  float lighting= lightingfront;
  float transparencyenabled = transparencyenabledfront;  
  matrix textureMatrix = matrix "current" (tmfront[0],tmfront[1],tmfront[2],tmfront[3],tmfront[4],tmfront[5],tmfront[6],tmfront[7],tmfront[8],tmfront[9],tmfront[10],tmfront[11],tmfront[12],tmfront[13],tmfront[14],tmfront[15]);
  string texturename = texturenamefront; 
  string reflectionmap = reflectionmapfront; 
  
  if(N.I>0){  
    Ka = Kaback; 
	Kd = Kdback; 
	Ks = Ksback; 
	Kr = Krback; 
	roughness = roughnessback; 
	reflectionBlend = reflectionBlendback;
	diffusecolor = diffusecolorback;
	specularcolor = specularcolorback;
    lighting= lightingback;
    transparencyenabled = transparencyenabledback;   
    textureMatrix = matrix "current" (tmback[0],tmback[1],tmback[2],tmback[3],tmback[4],tmback[5],tmback[6],tmback[7],tmback[8],tmback[9],tmback[10],tmback[11],tmback[12],tmback[13],tmback[14],tmback[15]);
    texturename = texturenameback; 
    reflectionmap = reflectionmapback; 
  }
  
 
  //from here like in defaultpolygonshader.sl
  
  
  // evaluate the texture map, if any
  if (texturename != ""){
	point a = point (s,t,0);
    // We don't use a matrix to pass in the texture matrix since
    // RenderMan automatically converts the textureMatrix into "current" space
    // so the point has also to be transformed into that space!
    // (And other rman renderers do other things with the matrix!)
	point p = transform( textureMatrix , a);
	float ss = xcomp(p);
	float tt = ycomp(p);
    if (debug != 0) printf("texture coords are %f %f",ss,tt);
	
	// look for optional transparency channel
	tr = float texture(texturename[3],ss,tt, "fill",1);
   
    Ct = color texture (texturename,ss, tt);
    // the following code should depend on an option such
    // as the OpenGL blend, modulate, replace, decal, etc options!
    // the following is possibly none of the openGL options.
    Ct = diffusecolor * Ct; 
  }
  else Ct = diffusecolor;

    // modulate the opacity by the alpha channel of the texture
    if (transparencyenabled != 0.0)
  	    Oi = Os*tr;
    else {
        if (tr==0)
            Oi=0;
        else
            Oi=1; //Os;  
    }

  if (lighting != 0)  {
        Nf = faceforward (normalize(N),I);
        V = -normalize(I);
        // calculate the diffuse component
        Ct = Ct * (Ka*ambient() + Kd*diffuse(Nf)) ;
  }

  // and add in the reflection map (like a specular highlight, without modulating by shading)
  if(raytracedreflections!=0){   
    vector reflDir = reflect(normalize(I),normalize(Nf));
    color Crefl = trace(P, reflDir);    
    Ct = (1-reflectionBlend) * Ct + reflectionBlend * Crefl;    
  } else{
    if (reflectionmap != "") {
      D = reflect(I, Nf) ;
      D = vtransform ("world", D);
	  Ct = (1-reflectionBlend) * Ct + reflectionBlend * color environment(reflectionmap, D);
	}
  } 


  // the surface color is a sum of ambient, diffuse, specular
  if (lighting != 0)
        Ci = Oi * ( Ct + specularcolor * Ks*myspecular(Nf,V,roughness) );
  else 
        Ci = Oi * Ct;


  if(raytracedvolumes!=0){
    color Crefr = trace(P, I); 
    Ci+=Crefr;
    Oi=1;
  }
}
