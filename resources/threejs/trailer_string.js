// COMMON_CODE_BLOCK_BEGIN
function textSpriteMaterial(message, parameters) {
    if ( parameters === undefined ) parameters = {};
    var fontface = "Helvetica";
    var fontsize = parameters.hasOwnProperty("fontsize") ? parameters["fontsize"] : 15;
    fontsize = fontsize*10;
    var lines = message.split('\\n');
    var size = 512;
    for(var i = 0; i<lines.length; i++){
        var tmp = lines[i].length;
        while(tmp*fontsize > size){
           fontsize--;
        }
    }
    
    var canvas = document.createElement('canvas');
    canvas.width = size;
    canvas.height = size;
    var context = canvas.getContext('2d');
    context.fillStyle = "rgba(255, 255, 255, 0)";
    context.fill();
    context.font = fontsize + "px " + fontface;
    
    // text color
    context.fillStyle = "rgba(0, 0, 0, 1.0)";
     for(var i = 0; i<lines.length; i++){
        context.fillText(lines[i], size/2, size/2+i*fontsize);
     }
    
    // canvas contents will be used for a texture
    var texture = new THREE.Texture(canvas);
    texture.needsUpdate = true;
    
    var spriteMaterial = new THREE.SpriteMaterial({map: texture, depthTest: true, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: 1 });
    return spriteMaterial;
}


// ---------------------- INITIALIZING OBJECTS--------------------------------------
// ---------------------------------------------------------------------------------

function init_object(obj) {
    if (obj.userData.hasOwnProperty("pointmaterial")) {
        init_points(obj);
        modelContains.points = true;
    }
    if (obj.userData.hasOwnProperty("pointlabels")) {
        init_pointlabels(obj);
        modelContains.pointlabels = true;
    }
    if (obj.userData.hasOwnProperty("edgematerial")) {
        init_lines(obj);
        modelContains.lines = true;
    }
    if (obj.userData.hasOwnProperty("edgelabels")) {
        init_edgelabels(obj);
        modelContains.edgelabels = true;
    }
    if (obj.userData.hasOwnProperty("arrowstyle")) {
        init_arrowheads(obj);
        modelContains.arrowheads = true;
    }
    if (obj.userData.hasOwnProperty("facetmaterial")) {
        init_faces(obj);
        modelContains.faces = true;
    }
}

function init_points(obj) {
    var pointgroup = new THREE.Group();
    pointgroup.name = "points";
    var points = obj.userData.points;
    var radii = obj.userData.pointradii;
    var materials = obj.userData.pointmaterial;
    var geometry,material;
    if (!Array.isArray(radii)) {
        geometry = new THREE.SphereBufferGeometry(radii);  
    }
    if (!Array.isArray(materials)) {
        material = materials;
    }
    for (var i=0; i<points.length; i++) {
        var point = points[i];
        if (Array.isArray(radii)) {
            if (radii[i] == 0) {
                continue;
            }
            geometry = new THREE.SphereBufferGeometry(radii[i]);  
        } 
        if (Array.isArray(materials)) {
            material = materials[i];     
        } 
        var sphere = new THREE.Mesh(geometry, material);
        point.addSphere(sphere);
        pointgroup.add(sphere);
    }
    obj.add(pointgroup);
}

function init_pointlabels(obj) {
    var points = obj.userData.points;
    var labels = obj.userData.pointlabels;
    var pointlabels = new THREE.Group();
    pointlabels.name = "pointlabels";
    if (Array.isArray(labels)) {
        for (var i=0; i<points.length; i++) {
            var point = points[i];
            var spriteMaterial = textSpriteMaterial( labels[i] );
	        var sprite = new THREE.Sprite(spriteMaterial);
            point.addLabel(sprite);
            pointlabels.add(sprite);
        }
    } else {
        var spriteMaterial = textSpriteMaterial( labels );
        for (var i=0; i<points.length; i++) {
            var point = points[i];
	        var sprite = new THREE.Sprite(spriteMaterial);
            point.addLabel(sprite);
            pointlabels.add(sprite);
        }
    }
    obj.add(pointlabels);
}

function init_lines(obj) {
    var edgeindices = obj.userData.edgeindices;
    var points = obj.userData.points;
    var materials = obj.userData.edgematerial;
    var geometry = new THREE.BufferGeometry();
    var bufarr = new Float32Array( obj.userData.edgeindices.length * 3 );
    var bufattr = new THREE.Float32BufferAttribute( bufarr, 3 );
    var geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', bufattr);
    if (Array.isArray(materials)) {     
        for (var i=0; i<materials.length; i++) {
            geometry.addGroup(2*i,2,i);
        }
    }
    var lines = new THREE.LineSegments(geometry, materials);
    lines.name = "lines";
    obj.add(lines);
    updateEdgesPosition(obj);
}

function init_edgelabels(obj) {
    var points = obj.userData.points;
    var edgeindices = obj.userData.edgeindices;
    var labels = obj.userData.edgelabels;
    var edgelabels = new THREE.Group();
    edgelabels.name = "edgelabels";
    if (Array.isArray(labels)) {
        for (var i=0; i<edgeindices.length/2; i++) {
            var spriteMaterial = textSpriteMaterial( labels[i] );
            var sprite = new THREE.Sprite(spriteMaterial);
            sprite.position.copy(new THREE.Vector3().addVectors(points[edgeindices[2*i]].vector,points[edgeindices[2*i+1]].vector).multiplyScalar(0.5));
            edgelabels.add(sprite);
        }
    } else {
        var spriteMaterial = textSpriteMaterial( labels );
        for (var i=0; i<edgeindices.length/2; i++) {
            var sprite = new THREE.Sprite(spriteMaterial);
            sprite.position.copy(new THREE.Vector3().addVectors(points[edgeindices[2*i]].vector,points[edgeindices[2*i+1]].vector).multiplyScalar(0.5));
            edgelabels.add(sprite);
        }
    }
    obj.add(edgelabels);
}

function init_arrowheads(obj) {
    var arrowheads = new THREE.Group();
    arrowheads.name = "arrowheads";
    var arrowstyle = obj.userData.arrowstyle;
    var edgeindices = obj.userData.edgeindices;
    var edgematerials = obj.userData.edgematerial;
    var points = obj.userData.points;
    var material;
    if (!Array.isArray(edgematerials)) {
        material = new THREE.MeshBasicMaterial( {color: edgematerials.color} );
    }

    for (var i=0; i<edgeindices.length; i=i+2) {
        var start = points[edgeindices[i]];
        var end = points[edgeindices[i+1]];
        var dist = start.vector.distanceTo( end.vector ) - start.radius() - end.radius();
        if (dist <= 0) {
            continue;
        }
        var dir = new THREE.Vector3().subVectors(end.vector,start.vector);
        dir.normalize();
        var axis = new THREE.Vector3().set(dir.z,0,-dir.x);
        axis.normalize();
        var radians = Math.acos( dir.y );
        var radius = dist/25;
        var height = dist/5;
        var geometry = new THREE.ConeBufferGeometry(radius,height);
        var position = new THREE.Vector3().addVectors(start.vector,dir.clone().multiplyScalar(start.radius()+dist-height/2));
        if (Array.isArray(edgematerials)) {
            material = new THREE.MeshBasicMaterial( {color: edgematerials[i].color} );
        }
        var cone = new THREE.Mesh( geometry, material );
        cone.quaternion.setFromAxisAngle(axis,radians);;
        cone.position.copy(position);;
        arrowheads.add(cone);
    }
    obj.add(arrowheads);
}

function init_faces(obj) {
    var points = obj.userData.points;
    var facets = obj.userData.facets;
    obj.userData.triangleindices = [];
    for (var i=0; i<facets.length; i++) {
        facet = facets[i];
        for (var t=0; t<facet.length-2; t++) {
            obj.userData.triangleindices.push(facet[0],facet[t+1],facet[t+2]);  
        }
    }
    var bufarr = new Float32Array( obj.userData.triangleindices.length * 3 );
    var bufattr = new THREE.Float32BufferAttribute(bufarr,3);
    
    var materials = obj.userData.facetmaterial;
    var geometry = new THREE.BufferGeometry();
    var frontmaterials = [];
    var backmaterials = [];
    geometry.setAttribute('position',bufattr);
    if (Array.isArray(materials)) {
        var tricount = 0;
        var facet;
        for (var i=0; i<facets.length; i++) {
            facet = facets[i];
            geometry.addGroup(tricount,(facet.length-2)*3,i);
            tricount += (facet.length-2)*3;
        }
        for (var j=0; j<materials.length; j++) {
            var fmat = materials[j].clone()
            fmat.side = THREE.FrontSide;
            frontmaterials.push(fmat);
            var bmat = materials[j].clone()
            bmat.side = THREE.BackSide;
            backmaterials.push(bmat);
            obj.userData.facetmaterial = frontmaterials.concat(backmaterials);
        }
    } else if (materials instanceof THREE.Material) {
        frontmaterials = materials.clone()
        frontmaterials.side = THREE.FrontSide;
        backmaterials = materials.clone()
        backmaterials.side = THREE.BackSide;
        obj.userData.facetmaterial = [frontmaterials, backmaterials];
    }
    // duplicating the object with front and back should avoid transparency issues
    var backmesh = new THREE.Mesh(geometry, backmaterials);
    // meshname is used to show/hide objects
    backmesh.name = "backfaces";
    obj.add(backmesh);
    var frontmesh = new THREE.Mesh(geometry, frontmaterials);
    frontmesh.name = "frontfaces";
    obj.add(frontmesh);
    updateFacesPosition(obj);
}
// //INITIALIZING


function updateFacesPosition(obj) {
    var points = obj.userData.points;
    var indices = obj.userData.triangleindices;
    var faces = obj.getObjectByName("frontfaces");
    var ba = faces.geometry.getAttribute("position");
    for (var i=0; i<indices.length; i++) {
        ba.setXYZ(i, points[indices[i]].vector.x, points[indices[i]].vector.y ,points[indices[i]].vector.z); 
    }
    faces.geometry.attributes.position.needsUpdate = true;
    
}

function updateEdgesPosition(obj) {
    var points = obj.userData.points;
    var indices = obj.userData.edgeindices;
    var lines = obj.getObjectByName("lines");
    var ba = lines.geometry.getAttribute("position"); 
    for (var i=0; i<indices.length; i++) {
        ba.setXYZ(i, points[indices[i]].vector.x, points[indices[i]].vector.y ,points[indices[i]].vector.z); 
    }
    lines.geometry.attributes.position.needsUpdate = true;
}

function onWindowResize() {
    renderer.setSize( three.clientWidth, three.clientHeight );
    svgRenderer.setSize( three.clientWidth, three.clientHeight );
    updateCamera();
}

function updateCamera() {
    var width = three.clientWidth;
    var height = three.clientHeight;
    var aspect = width / height;
    if (camera.type == "OrthographicCamera") {
        camera.left = frustumSize * aspect / - 2;
        camera.right = frustumSize * aspect / 2;
        camera.top = frustumSize / 2;
        camera.bottom = - frustumSize / 2;
    } else if (camera.type == "PerspectiveCamera") {
        camera.aspect = aspect;
    }
    camera.updateProjectionMatrix();
}

function changeCamera(event) {
    var selindex = event.currentTarget.selectedIndex;
    camera = cameras[selindex];
    control = controls[selindex];
    control.enabled = true; 
    for (var i=0; i<controls.length; i++) {
        if (i!=selindex) {
            controls[i].enabled = false;
        }
    }
    updateCamera();
}

var camtypenode = document.getElementById('cameraType_OUTPUTID');
camtypenode.onchange = changeCamera;
camtypenode.dispatchEvent(new Event('change'));

onWindowResize();
window.addEventListener('resize', onWindowResize);	


var xRotationEnabled = false;
var yRotationEnabled = false;
var zRotationEnabled = false;
var rotationSpeedFactor = 1;
var settingsShown = false;
var labelsShown = true;
var intervals = [];
var timeouts = [];
var explodingSpeed = 0.05;
var explodeScale = 0.000001;
var XMLS = new XMLSerializer();
var svgElement;
var renderId;

var render = function () {

	renderId = requestAnimationFrame(render);

//	comment in for automatic explosion
//	explode(updateFactor());

    var phi = 0.02 * rotationSpeedFactor;

    if (xRotationEnabled) {
        scene.rotation.x += phi;
    }
    if (yRotationEnabled) {
        scene.rotation.y += phi;
    }
    if (zRotationEnabled) {
        scene.rotation.z += phi;
    }

    control.update();
    renderer.render(scene, camera);
};

if ( THREE.WEBGL.isWebGLAvailable() ) {
	render();
} else {
	var warning = WEBGL.getWebGLErrorMessage();
	three.appendChild( warning );
}
    
function changeTransparency() {
    var opacity = 1-Number(event.currentTarget.value);
    for (var i=0; i<scene.children.length; i++) {
        child = scene.children[i];
        if ( child.userData.hasOwnProperty("facetmaterial") ) {
            if (Array.isArray(child.userData.facetmaterial)) {
                for (var j=0; j<child.userData.facetmaterial.length; j++) {
                    child.userData.facetmaterial[j].opacity = opacity;
                }
            } else {
                child.userData.facetmaterial.opacity = opacity;
            }    
        }
    }
}

function changeRotationX(event){
    xRotationEnabled = event.currentTarget.checked;
}	

function changeRotationY(event){
    yRotationEnabled = event.currentTarget.checked;
}	

function changeRotationZ(event){
    zRotationEnabled = event.currentTarget.checked;
}	


function changeRotationSpeedFactor(event){
    rotationSpeedFactor = Number(event.currentTarget.value);
}

function resetScene(){
    scene.rotation.set(0,0,0);
    camera.position.set(0,0,5);
    camera.up.set(0,1,0);
}

function showSettings(event){
    document.getElementById('settings_OUTPUTID').style.visibility = 'visible';
    document.getElementById('showSettingsButton_OUTPUTID').style.visibility = 'hidden';
    document.getElementById('hideSettingsButton_OUTPUTID').style.visibility = 'visible';
    settingsShown = true;
}

function hideSettings(event){
    document.getElementById('settings_OUTPUTID').style.visibility = 'hidden';
    document.getElementById('showSettingsButton_OUTPUTID').style.visibility = 'visible';
    document.getElementById('hideSettingsButton_OUTPUTID').style.visibility = 'hidden';
    settingsShown = false;
}



var pos = 150* Math.PI;

function updateFactor() {
    pos++;
    return Math.sin(.01*pos)+1;
}

// ------------------------ FOLDING ------------------------------------------------
// ---------------------------------------------------------------------------------
// rotate point p around axis defined by points p1 and p2 by given angle
function rotate(p, p1, p2, angle ){   
    angle = -angle;
    var x = p.x, y = p.y, z = p.z, 
    a = p1.x, b = p1.y, c = p1.z, 
    u = p2.x-p1.x, v = p2.y-p1.y, w = p2.z-p1.z;
    var result = [];
    var L = u*u + v*v + w*w;
    var sqrt = Math.sqrt;
    var cos = Math.cos;
    var sin = Math.sin;

    result[0] = ((a*(v*v+w*w)-u*(b*v+c*w-u*x-v*y-w*z))*(1-cos(angle))+L*x*cos(angle)+sqrt(L)*(-c*v+b*w-w*y+v*z)*sin(angle))/L;
    result[1] = ((b*(u*u+w*w)-v*(a*u+c*w-u*x-v*y-w*z))*(1-cos(angle))+L*y*cos(angle)+sqrt(L)*(c*u-a*w+w*x-u*z)*sin(angle))/L;
    result[2] = ((c*(u*u+v*v)-w*(a*u+b*v-u*x-v*y-w*z))*(1-cos(angle))+L*z*cos(angle)+sqrt(L)*(-b*u+a*v-v*x+u*y)*sin(angle))/L;

    return result;
}

var fold = function(event){
    var obj = foldables[Number(event.currentTarget.name)];
    var foldvalue = Number(event.currentTarget.value);
    var scale = foldvalue - obj.userData.oldscale;

    for (var j=0; j<obj.userData.axes.length; j++) {
        rotateVertices(obj, j, scale);
    }
    update(obj);
    obj.userData.oldscale += scale;
    lookAtBarycenter(obj);
}

function lookAtBarycenter(obj){
    control.target = barycenter(obj);
}

function barycenter(obj) {
    var center = new THREE.Vector3(0,0,0);
    var points = obj.userData.points;
    for (var i=0; i<points.length; i++){
        center.add(points[i].vector);
    }
    center.divideScalar(points.length);
    return center;
}

function rotateVertices(obj, edge, scale) {
    var axes = obj.userData.axes;
    var subtrees = obj.userData.subtrees;
    var points = obj.userData.points;
    var angles = obj.userData.angles;
    if (edge < axes.length){
        for (var j=0; j<subtrees[edge].length; j++){
            var rotP = rotate(points[subtrees[edge][j]].vector, points[axes[edge][0]].vector,points[axes[edge][1]].vector, scale * (Math.PI - angles[edge]));
            points[subtrees[edge][j]].set(rotP[0],rotP[1],rotP[2]);
        }
    }
}

function update(obj) {
   updateFacesPosition(obj);
   updateEdgesPosition(obj);
}

if (foldables.length) {
    var settings = document.getElementById('settings_OUTPUTID');
    var foldDiv = document.createElement('div');
    foldDiv.id = 'fold_OUTPUTID';
    var title = document.createElement('strong');
    title.innerHTML = 'Fold';
    foldDiv.appendChild(title);
    foldDiv.className = 'group';
    for (var i=0; i<foldables.length; i++) {
        var range = document.createElement('input');
        range.type = 'range';
        range.min = 0;
        range.max = 1;
        range.value = 0;
        range.step = 0.001;
        range.name = String(i);
        range.oninput = fold;
        foldDiv.appendChild(range);
    }
    lookAtBarycenter(foldables[0]);
    settings.insertBefore(foldDiv,settings.childNodes[0]);
}

    
// ---------------------- EXPLOSION ------------------------------------------------
// ---------------------------------------------------------------------------------

if (explodableModel) {
    for (var i=0; i<scene.children.length; i++) {
        obj = scene.children[i];
        if ( obj.userData.explodable ) {
            computeCentroid(obj);
        }
    }
    document.getElementById('explodeRange_OUTPUTID').oninput = triggerExplode;
    document.getElementById('explodeCheckbox_OUTPUTID').onchange = triggerAutomaticExplode;
    document.getElementById('explodingSpeedRange_OUTPUTID').oninput = setExplodingSpeed;
    explode(0.000001);
}

function computeCentroid(obj) {
    centroid = new THREE.Vector3();
    obj.userData.points.forEach(function(pmpoint) {
        centroid.add(pmpoint.vector);			
    });
    centroid.divideScalar(obj.userData.points.length);
    obj.userData.centroid = centroid;
}

function explode(factor) {
    for (var i=0; i<scene.children.length; i++) {
        var obj = scene.children[i];
        if (obj.userData.hasOwnProperty("centroid")) { 
            var c = obj.userData.centroid;
            obj.position.set(c.x*factor, c.y*factor, c.z*factor);
        }
    }	
}

function triggerExplode(event){
    explodeScale = Number(event.currentTarget.value);
    explode(explodeScale);
}

function setExplodingSpeed(event){
    explodingSpeed = Number(event.currentTarget.value);
}

function triggerAutomaticExplode(event){
    if (event.currentTarget.checked){
        startExploding();
    } else {
        clearIntervals();
    }	
}

function startExploding(){
    intervals.push(setInterval(explodingInterval, 25));
}


function explodingInterval(){
    explodeScale += explodingSpeed;
    if (explodeScale <= 6){ 
        explode(explodeScale);
    }
    else{
        explode(6);
        explodeScale = 6;
        clearIntervals();
        timeouts.push(setTimeout(startUnexploding, 3000));
    }
    document.getElementById('explodeRange_OUTPUTID').value = explodeScale;
}


function startUnexploding(){
    intervals.push(setInterval(unexplodingInterval, 25));
}

function unexplodingInterval(){
    explodeScale -= explodingSpeed;
    if (explodeScale >= 0){	
        explode(explodeScale);
    }
    else {
        explode(0);
        explodeScale = 0;
        clearIntervals();
        timeouts.push(setTimeout(startExploding, 3000));
    }
    document.getElementById('explodeRange_OUTPUTID').value = explodeScale;
}

function clearIntervals(){
    intervals.forEach(function(interval){
        clearInterval(interval);
    });
    intervals = [];
    timeouts.forEach(function(timeout){
        clearTimeout(timeout);
    });
    timeouts = [];
}

// ---------------------- DISPLAY --------------------------------------------------
// ---------------------------------------------------------------------------------

const objectTypeInnerHTMLs = { points: "Points", pointlabels: "Point labels", lines: "Edges", edgelabels: "Edge labels", faces: "Faces", arrowheads: "Arrow heads" };
const objectTypeVisible = {};
Object.assign(objectTypeVisible,modelContains);
const sortedObjectTypeKeys = Object.keys(objectTypeInnerHTMLs).sort();
const shownObjectTypesList = document.getElementById('shownObjectTypesList_OUTPUTID');

function setVisibility(bool,objname) {
    for (var i=0; i<scene.children.length; i++){
        var obj = scene.children[i].getObjectByName(objname);
        if (obj) {
            obj.visible = bool;
        }
    }
}

function toggleObjectTypeVisibility(event){
    var name = event.currentTarget.name;
    var checked = event.currentTarget.checked;
    objectTypeVisible[name] = checked;
    if (name == "faces") {
        setVisibility(checked,"frontfaces");
        setVisibility(checked,"backfaces");
    } else {
        setVisibility(checked,name);
    }
}

for (var i=0; i<sortedObjectTypeKeys.length; i++){
    var key = sortedObjectTypeKeys[i];
    if (modelContains[key]) {
        var objTypeNode = document.createElement('span');
        objTypeNode.innerHTML = objectTypeInnerHTMLs[key] + '<br>';
        var checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = true;
        checkbox.name = key;
        checkbox.onchange = toggleObjectTypeVisibility;
        shownObjectTypesList.appendChild(checkbox);
        shownObjectTypesList.appendChild(objTypeNode);
    }
}

// ------------------------------------------------------

function toggleObjectVisibility(event){
    var nr = Number(event.currentTarget.name);
    scene.children[nr].visible = event.currentTarget.checked;
}

// append checkboxes for displaying or hiding objects
var shownObjectsList = document.getElementById('shownObjectsList_OUTPUTID');
for (var i=0; i<scene.children.length; i++){
    obj = scene.children[i];
    var objNode = document.createElement('span');
    objNode.innerHTML = obj.name + '<br>';
    var checkbox = document.createElement('input');
    checkbox.type = 'checkbox';
    checkbox.checked = true;
    checkbox.name = String(i);
    checkbox.onchange = toggleObjectVisibility;
    shownObjectsList.appendChild(checkbox);
    shownObjectsList.appendChild(objNode);
}

// ---------------------- SVG ------------------------------------------------------
// ---------------------------------------------------------------------------------

function takeSvgScreenshot() {
    if (objectTypeVisible["pointlabels"]) {
        setVisibility(false,"pointlabels");
    }
    if (objectTypeVisible["edgelabels"]) {
        setVisibility(false,"edgelabels");
    }
    svgRenderer.render(scene,camera);
    svgElement = XMLS.serializeToString(svgRenderer.domElement);
    
    if (objectTypeVisible["pointlabels"]) {
        setVisibility(true,"pointlabels");
    }
    if (objectTypeVisible["edgelabels"]) {
        setVisibility(true,"edgelabels");
    }

    if (document.getElementById('tab_OUTPUTID').checked){
        //show in new tab
        var myWindow = window.open("","");
        myWindow.document.body.innerHTML = svgElement;
    } else{
        // download svg file 
        download("screenshot.svg", svgElement);
    }
}

function download(filename, text) {
    var element = document.createElement('a');
    element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
    element.setAttribute('download', filename);

    element.style.display = 'none';
    document.body.appendChild(element);

    element.click();

    document.body.removeChild(element);
}


document.getElementById('transparencyRange_OUTPUTID').oninput = changeTransparency;
document.getElementById('changeRotationX_OUTPUTID').onchange = changeRotationX;
document.getElementById('changeRotationY_OUTPUTID').onchange = changeRotationY;
document.getElementById('changeRotationZ_OUTPUTID').onchange = changeRotationZ;
document.getElementById('resetButton_OUTPUTID').onclick = resetScene;
document.getElementById('rotationSpeedRange_OUTPUTID').oninput = changeRotationSpeedFactor;
document.getElementById('takeScreenshot_OUTPUTID').onclick = takeSvgScreenshot;
document.getElementById('showSettingsButton_OUTPUTID').onclick = showSettings;
document.getElementById('hideSettingsButton_OUTPUTID').onclick = hideSettings;


// ------------------ SHORTCUTS --------------------------------------------
// -------------------------------------------------------------------------

/**
 * http://www.openjs.com/scripts/events/keyboard_shortcuts/
 * Version : 2.01.B
 * By Binny V A
 * License : BSD
 */
shortcut = {
	'all_shortcuts':{},//All the shortcuts are stored in this array
	'add': function(shortcut_combination,callback,opt) {
		//Provide a set of default options
		var default_options = {
			'type':'keydown',
			'propagate':false,
			'disable_in_input':false,
			'target':document,
			'keycode':false
		}
		if(!opt) opt = default_options;
		else {
			for(var dfo in default_options) {
				if(typeof opt[dfo] == 'undefined') opt[dfo] = default_options[dfo];
			}
		}

		var ele = opt.target;
		if(typeof opt.target == 'string') ele = document.getElementById(opt.target);
		var ths = this;
		shortcut_combination = shortcut_combination.toLowerCase();

		//The function to be called at keypress
		var func = function(e) {
			e = e || window.event;
			
			if(opt['disable_in_input']) { //Don't enable shortcut keys in Input, Textarea fields
				var element;
				if(e.target) element=e.target;
				else if(e.srcElement) element=e.srcElement;
				if(element.nodeType==3) element=element.parentNode;

				if(element.tagName == 'INPUT' || element.tagName == 'TEXTAREA') return;
			}
	
			//Find Which key is pressed
			if (e.keyCode) code = e.keyCode;
			else if (e.which) code = e.which;
			var character = String.fromCharCode(code).toLowerCase();
			
			if(code == 188) character=","; //If the user presses , when the type is onkeydown
			if(code == 190) character="."; //If the user presses , when the type is onkeydown

			var keys = shortcut_combination.split("+");
			//Key Pressed - counts the number of valid keypresses - if it is same as the number of keys, the shortcut function is invoked
			var kp = 0;
			
			//Work around for stupid Shift key bug created by using lowercase - as a result the shift+num combination was broken
			var shift_nums = {
				"`":"~",
				"1":"!",
				"2":"@",
				"3":"#",
				"4":"$",
				"5":"%",
				"6":"^",
				"7":"&",
				"8":"*",
				"9":"(",
				"0":")",
				"-":"_",
				"=":"+",
				";":":",
				"'":"\"",
				",":"<",
				".":">",
				"/":"?",
				"\\":"|"
			}
			//Special Keys - and their codes
			var special_keys = {
				'esc':27,
				'escape':27,
				'tab':9,
				'space':32,
				'return':13,
				'enter':13,
				'backspace':8,
	
				'scrolllock':145,
				'scroll_lock':145,
				'scroll':145,
				'capslock':20,
				'caps_lock':20,
				'caps':20,
				'numlock':144,
				'num_lock':144,
				'num':144,
				
				'pause':19,
				'break':19,
				
				'insert':45,
				'home':36,
				'delete':46,
				'end':35,
				
				'pageup':33,
				'page_up':33,
				'pu':33,
	
				'pagedown':34,
				'page_down':34,
				'pd':34,
	
				'left':37,
				'up':38,
				'right':39,
				'down':40,
	
				'f1':112,
				'f2':113,
				'f3':114,
				'f4':115,
				'f5':116,
				'f6':117,
				'f7':118,
				'f8':119,
				'f9':120,
				'f10':121,
				'f11':122,
				'f12':123
			}
	
			var modifiers = { 
				shift: { wanted:false, pressed:false},
				ctrl : { wanted:false, pressed:false},
				alt  : { wanted:false, pressed:false},
				meta : { wanted:false, pressed:false}	//Meta is Mac specific
			};
                        
			if(e.ctrlKey)	modifiers.ctrl.pressed = true;
			if(e.shiftKey)	modifiers.shift.pressed = true;
			if(e.altKey)	modifiers.alt.pressed = true;
			if(e.metaKey)   modifiers.meta.pressed = true;
                        
			for(var i=0; k=keys[i],i<keys.length; i++) {
				//Modifiers
				if(k == 'ctrl' || k == 'control') {
					kp++;
					modifiers.ctrl.wanted = true;

				} else if(k == 'shift') {
					kp++;
					modifiers.shift.wanted = true;

				} else if(k == 'alt') {
					kp++;
					modifiers.alt.wanted = true;
				} else if(k == 'meta') {
					kp++;
					modifiers.meta.wanted = true;
				} else if(k.length > 1) { //If it is a special key
					if(special_keys[k] == code) kp++;
					
				} else if(opt['keycode']) {
					if(opt['keycode'] == code) kp++;

				} else { //The special keys did not match
					if(character == k) kp++;
					else {
						if(shift_nums[character] && e.shiftKey) { //Stupid Shift key bug created by using lowercase
							character = shift_nums[character]; 
							if(character == k) kp++;
						}
					}
				}
			}
			
			if(kp == keys.length && 
						modifiers.ctrl.pressed == modifiers.ctrl.wanted &&
						modifiers.shift.pressed == modifiers.shift.wanted &&
						modifiers.alt.pressed == modifiers.alt.wanted &&
						modifiers.meta.pressed == modifiers.meta.wanted) {
				callback(e);
	
				if(!opt['propagate']) { //Stop the event
					//e.cancelBubble is supported by IE - this will kill the bubbling process.
					e.cancelBubble = true;
					e.returnValue = false;
	
					//e.stopPropagation works in Firefox.
					if (e.stopPropagation) {
						e.stopPropagation();
						e.preventDefault();
					}
					return false;
				}
			}
		}
		this.all_shortcuts[shortcut_combination] = {
			'callback':func, 
			'target':ele, 
			'event': opt['type']
		};
		//Attach the function with the event
		if(ele.addEventListener) ele.addEventListener(opt['type'], func, false);
		else if(ele.attachEvent) ele.attachEvent('on'+opt['type'], func);
		else ele['on'+opt['type']] = func;
	},

	//Remove the shortcut - just specify the shortcut and I will remove the binding
	'remove':function(shortcut_combination) {
		shortcut_combination = shortcut_combination.toLowerCase();
		var binding = this.all_shortcuts[shortcut_combination];
		delete(this.all_shortcuts[shortcut_combination])
		if(!binding) return;
		var type = binding['event'];
		var ele = binding['target'];
		var callback = binding['callback'];

		if(ele.detachEvent) ele.detachEvent('on'+type, callback);
		else if(ele.removeEventListener) ele.removeEventListener(type, callback, false);
		else ele['on'+type] = false;
	}
}

shortcut.add("Alt+Left",function() {
	var event = new Event('click');
	if (settingsShown){
		document.getElementById('hideSettingsButton_OUTPUTID').dispatchEvent(event);
	} else {
		document.getElementById('showSettingsButton_OUTPUTID').dispatchEvent(event);
	}
});


// COMMON_CODE_BLOCK_END
