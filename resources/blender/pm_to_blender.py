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
parser.add_argument("-t", "--transparency", action='store_true', 
                    help="Enables transparency for all imported objects.")
parser.add_argument("-d", "--deselect", action='store_true', 
                    help="Deselect everything") 
parser.add_argument("-f", "--file", dest="filepath", required=True,
                    help="X3D file to import")

args = parser.parse_args(argv)

ct = bpy.context
# import our created x3d file into a new scene
bpy.ops.scene.new(type='NEW')
ct.scene.name = 'polymake'
bpy.ops.import_scene.x3d(filepath=args.filepath)

# if you use blender as a viewer only, the following might be helpful
def enable_transparency():
    for obj in ct.scene.objects[:]:
        if obj.select_get() == True:
            obj.show_transparent = True                 # this enables transparency in a 3D VIEW (without "rendering")
            if obj.name == "Viewpoint":
                ct.scene.camera = obj

def deselect():
    for obj in ct.scene.objects[:]:
        obj.select_set(False)
if args.transparency: enable_transparency()
if args.deselect: deselect()
