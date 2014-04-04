//author Benjamin Kutschan

#version 330

uniform sampler2D texFront;
uniform sampler2D texRight;
uniform sampler2D texLeft;
uniform sampler2D texBack;
uniform sampler2D texTop;
uniform sampler2D texFloor;

uniform float angle;

in vec2 tex_Coord2;
 
void main(void)
{
  //float angle = 90;
  //float s = gl_TexCoord[0].s;
  //float t = gl_TexCoord[0].t;
  float s = tex_Coord2.s;
  float t = tex_Coord2.t;
  float R = sqrt(s*s + t*t);
  float alpha = R*3.141592/2*angle/180;
  //if we are insid the dome master circle
  if(R<=1){
    float x = sin(alpha)*s/R;
    float y = sin(alpha)*t/R;
    float z = cos(alpha);
    //we are in the front texture
    if(z>abs(x) && z>abs(y)){
    	float X = x/z;
    	float Y = y/z;
    	gl_FragColor = texture( texFront, vec2(X/2+.5,Y/2+.5));
    //we are in the back texture
    }else if(z < -abs(x) && z < -abs(y)){
    	float X = x/z;
    	float Y = -y/z;
    	gl_FragColor = texture( texBack, vec2(X/2+.5,Y/2+.5));
    //in floor or top texture
    }else if(abs(y)>abs(x)){
    	//floor
    	if(y<0){
    		float X = -x/y;
    		float Z = -z/y;
    		gl_FragColor = texture( texFloor, vec2(X/2+.5,Z/2+.5));
    	//top
    	}else{
    		float X = x/y;
    		float Z = z/y;
    		gl_FragColor = texture( texTop, vec2(X/2+.5,-Z/2+.5));
    	}
    //left or right texture
    }else{
    	//right
    	if(x>0){
    		float Y = y/x;
    		float Z = z/x;
    		gl_FragColor = texture( texRight, vec2(-Z/2+.5,Y/2+.5));
    	//left
    	}else{
    		float Y = -y/x;
    		float Z = -z/x;
    		gl_FragColor = texture( texLeft, vec2(Z/2+.5,Y/2+.5));
    	}
    }
  }else{
  	gl_FragColor = vec4(0.0,0.0,0.0,1.0);
  }
 
}