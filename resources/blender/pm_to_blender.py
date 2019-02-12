import argparse
import bpy, sys

def set_screen_to(screen_name):
    context = bpy.context
    to_screen = bpy.data.screens.find(screen_name)
    if to_screen >= 0:
        # context.screen
        from_screen = bpy.data.screens.find(context.screen.name)
        delta = to_screen - from_screen
        sgn = abs(delta) / delta
        delta = abs(delta)
        for i in range(delta):
           bpy.ops.screen.screen_set('INVOKE_DEFAULT', delta=sgn)
        print("pm_to_blender: screen is now set to " + screen_name)
    else :
        print("can't find screen" + screen_name)
    context.screen.scene = bpy.data.scenes['polymake']

argv = sys.argv
if "--" not in argv:
    argv = []
else:
    argv = argv[argv.index("--") + 1:]

# set the prog name to match real usage
parser = argparse.ArgumentParser(
    # the normal -h flag is problematic, it shows the cycles render help. changed it to -i/--info
    add_help=False,              
    prefix_chars = '-+',             
    description = 'Import x3d file to blender.',
    prog = "blender -P "+__file__+" --",
)
parser.add_argument('-i', '--info', action='help', default=argparse.SUPPRESS,
                    help='Show this info message and exit.')
parser.add_argument("-v", "--viewer", action='store_true', 
                    help="Use blender as viewer. This changes the screen to 3D View Full, deselects imported objects and enables transparency.")
parser.add_argument("-q", "--quadview", action='store_true', 
                    help="Splits 3D view region into 4 parts. Toggle with ctrl+alt+q. Only with -v flag") 
parser.add_argument("-m", "--maximize", action='store_true', 
                    help="Maximize the 3D view window region. Toggle with alt+f10. Only with -v flag") 
parser.add_argument("-c", "--cameraview", action='store_true', 
                    help="Change view to scene camera (the x3d Viewpoint). Only with -v flag") 
parser.add_argument("-f", "--file", dest="filepath", required=True,
                    help="X3D file to import")

args = parser.parse_args(argv)

ct = bpy.context
# import our created x3d file into a new scene
bpy.ops.scene.new(type='NEW')
ct.scene.name = 'polymake'
bpy.ops.import_scene.x3d(filepath=args.filepath)

# if you use blender as a viewer only, the following might be helpful
def as_viewer():
    for obj in ct.scene.objects[:]:
        if obj.select == True:
            obj.show_transparent = True                 # this enables transparency in a 3D VIEW (without "rendering")
            bpy.data.objects[obj.name].select = False
            if obj.name == "Viewpoint":
                ct.scene.camera = obj
    
    # almost white 3D view background 
    bpy.context.user_preferences.themes[0].view_3d.space.gradients.high_gradient = (0.9,0.9,0.9)
    
    set_screen_to("3D View Full")

    # viewnumpad and quadview need a context override
    viewer_ct = bpy.context.copy()
    for area in ct.screen.areas:
        if area.type == 'VIEW_3D':
            viewer_ct['area'] = area
            viewer_ct['space_data'] = area.spaces[0]
            for region in area.regions:
                if region.type == 'WINDOW':
                    viewer_ct['region'] = region
    
    if args.cameraview: bpy.ops.view3d.viewnumpad(viewer_ct, 'EXEC_DEFAULT', type='CAMERA')
    if args.quadview: bpy.ops.screen.region_quadview(viewer_ct)  
    if args.maximize: bpy.ops.screen.screen_full_area(viewer_ct, use_hide_panels=True)

if args.viewer: as_viewer()
