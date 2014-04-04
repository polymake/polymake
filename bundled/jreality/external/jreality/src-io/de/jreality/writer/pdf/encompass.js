bb = scene.computeBoundingBox();	

//--------------------------
// calculate dimensions of the bounding box
//--------------------------
xDist = bb.max.x - bb.min.x;
yDist = bb.max.y - bb.min.y;
zDist = bb.max.z - bb.min.z;

//--------------------------
// scale the distance between camera and target object
//--------------------------
a = 0;
b = 0;

if (xDist > yDist)
	a = xDist;
else 
	a = yDist;
	
b = a / Math.tan(camera.fov);
dist = b + zDist/2;

if(dist > 2*bb.max.subtract(bb.min).length){
	dist = bb.max.subtract(bb.min).length;
} 

pos = new Vector3(dist, 0, 0).add(bb.center);
camera.position.set(pos);
camera.targetPosition.set(bb.center);
scene.update();

