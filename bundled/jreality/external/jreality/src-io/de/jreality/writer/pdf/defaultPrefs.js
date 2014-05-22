var canvas = null;
var myEventHdlr = new RenderEventHandler();
myEventHdlr.onEvent = function(event)
{
	runtime.removeEventHandler(myEventHdlr);
	canvas = event.canvas;
	camera = scene.cameras.getByName("##cam##");
	if (camera != null) {
		bb = scene.computeBoundingBox();
		camera.targetPosition.set(bb.center);
		canvas.setCamera(camera);
	}
	upperColor = new Color(##backgroundColorUpper##);
	lowerColor = new Color(##backgroundColorLower##);
	canvas.background.setColor(upperColor, lowerColor);
}
runtime.addEventHandler(myEventHdlr);
runtime.setCurrentTool("##tool##");

scene.renderMode = "##render_mode##";
scene.gridMode = "##grid_mode##";
scene.showAxes = ##show_axes##;
scene.showGrid = ##show_grid##;
scene.renderDoubleSided = true;
//scene.smoothing = true;
//scene.smoothingAngle = 90;
//scene.smoothingOverride = true;
scene.lightScaleFactor = 1.0;
scene.lightScheme = "##lighting##";