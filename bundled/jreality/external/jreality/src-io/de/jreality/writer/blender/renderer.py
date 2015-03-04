import bpy
from mathutils import Matrix
from mathutils import Vector
from mathutils import Euler
import xml.etree.ElementTree as ET
import math
import zlib
import base64

tagToObject = {}
tagToMaterial = {}
tagToTexture = {}
materialStack = []
transformStack = []
sphereMaterials = {}
tubeMaterials = {}

urSphere = None
urCylinder = None

def createNURBSSphereData(name):
    bpy.ops.surface.primitive_nurbs_surface_sphere_add()
    sphereObject = bpy.context.scene.objects.active
    sphereSurface = sphereObject.data
    bpy.context.scene.objects.unlink(sphereObject)
    bpy.data.objects.remove(sphereObject)
    sphereSurface.name = name
    sphereSurface.materials.append(None)
    return sphereSurface


def createNURBSCylinderData(name):
    bpy.ops.surface.primitive_nurbs_surface_cylinder_add()
    cylinderObject = bpy.context.scene.objects.active
    cylinderSurface = cylinderObject.data
    bpy.context.scene.objects.unlink(cylinderObject)
    bpy.data.objects.remove(cylinderObject)
    cylinderSurface.name = name
    cylinderSurface.materials.append(None)
    return cylinderSurface

def getStandardSphere():
    global urSphere 
    if urSphere is None:
        urSphere = createNURBSSphereData('Sphere')
    return urSphere    

def getStandardCylinder(): 
    global urCylinder
    if urCylinder is None:
        urCylinder = createNURBSCylinderData('Cylinder')
    return urCylinder

def parseMatrix(tag):
    mm = [float(mij) for mij in tag.text.split()]
    return Matrix((mm[0:4], mm[4:8], mm[8:12], mm[12:16]))


def parseColor(treeRoot, tag, rootPath):
    tag = resolveReference(treeRoot, tag, rootPath);
    if tag is None: return [1.0, 1.0, 1.0]
    r = float(tag.find('red').text) / 255.0
    g = float(tag.find('green').text) / 255.0
    b = float(tag.find('blue').text) / 255.0
    return [r, g, b]

def parseGeometryAttribute(tag, count, isFloat, normalizeTo3Components):
    if tag is None: return []
    if isFloat: dataTags = tag.findall('double-array')
    else: dataTags = tag.findall('int-array')
    if dataTags:
        if isFloat:
            data = [[float(f) for f in dataTag.text.split()] for dataTag in dataTags]
        else:
            data = [[int(i) for i in dataTag.text.split()] for dataTag in dataTags]
    else:
        if tag.text is None:
            data = []
        else:
            if isFloat:
                dataInline = [float(f) for f in tag.text.split()]
            else:
                dataInline = [int(i) for i in tag.text.split()]    
            l = int(len(dataInline) / count);  
            data = [dataInline[i*l : i*l+l] for i in range(0, count)]
    if normalizeTo3Components:
        data = [normalizeTo3D(vec) for vec in data]
    return data    

def normalizeTo3D(vec):
    l = int(len(vec))
    if l == 4: 
        if vec[-1] != 0.0:
            return [vi/vec[-1] for vi in vec[0:3]]
        else:
            return vec[0:3]
    if l == 3: return vec
    if l == 2: return [vec[0], vec[1], 0.0] 
    if l == 1: return [vec[0], 0.0, 0.0]
    return None

def createMesh(tag):
    name = tag.find('name').text;
    mesh = bpy.data.meshes.new(name)
    # parse vertices
    vertexAttributes = tag.find('vertexAttributes')
    vertexAttributesSize = int(vertexAttributes.get('size'));
    vertexDataTag = vertexAttributes.find("DataList[@attribute='coordinates']")
    vertexData = parseGeometryAttribute(vertexDataTag, vertexAttributesSize, True, True)
    # parse edges
    edgeData = []
    edgeAttributes = tag.find('edgeAttributes')
    if edgeAttributes is not None:
        edgeAttributesSize = int(edgeAttributes.get('size'));
        edgeDataTag = edgeAttributes.find("DataList[@attribute='indices']")
        rawEdgeData = parseGeometryAttribute(edgeDataTag, edgeAttributesSize, False, False)
        edgeData = []
        for edges in rawEdgeData:
            if len(edges) == 2:
                edgeData.append(edges)
            else: # split edge sequence into single edges
                for ie in range(0, len(edges) - 1): 
                    edgeData.append([edges[ie], edges[ie + 1]])
    # parse faces
    faceData = []
    faceAttributes = tag.find('faceAttributes')
    if faceAttributes is not None:
        faceAttributesSize = int(faceAttributes.get('size'));
        faceDataTag = tag.find("faceAttributes/DataList[@attribute='indices']")
        faceData = parseGeometryAttribute(faceDataTag, faceAttributesSize, False, False)
    # create mesh
    mesh.from_pydata(vertexData, edgeData, faceData)
    
    # vertex colors
    vertexColorsTag = vertexAttributes.find("DataList[@attribute='colors']")
    if vertexColorsTag is not None:
        vertexColors = parseGeometryAttribute(vertexColorsTag, vertexAttributesSize, True, True)
        mesh['vertexColors'] = vertexColors
        colorLayer = mesh.vertex_colors.new(name='Vertex Colors')
        colorIndex = 0
        for poly in mesh.polygons:
            for vertexIndex in poly.vertices:
                colorLayer.data[colorIndex].color = vertexColors[vertexIndex]
                colorIndex += 1
    
    # relative point radii
    vertexRadiiTag = vertexAttributes.find("DataList[@attribute='relativeRadii']")
    if vertexRadiiTag is not None:
        vertexRadii = parseGeometryAttribute(vertexRadiiTag, vertexAttributesSize, True, False)
        mesh['relativePointRadii'] = vertexRadii
    
    # edge colors
    edgeColorsTag = tag.find("edgeAttributes/DataList[@attribute='colors']")
    if edgeColorsTag is not None:
        rawEdgeColors = parseGeometryAttribute(edgeColorsTag, edgeAttributesSize, True, True)
        colorIndex = 0
        edgeColors = []
        for edgeList in rawEdgeData: 
            edgeColors.extend([rawEdgeColors[colorIndex] for i in range(0,len(edgeList) - 1)])
            colorIndex += 1
        mesh['edgeColors'] = edgeColors
        
    # relative line radii
    lineRadiiTag = tag.find("edgeAttributes/DataList[@attribute='relativeRadii']")
    if lineRadiiTag is not None:
        rawLineRadii = parseGeometryAttribute(lineRadiiTag, edgeAttributesSize, True, False)
        radiusIndex = 0
        lineRadii = []
        for edgeList in rawEdgeData:
            lineRadii.extend([rawLineRadii[radiusIndex] for i in range(0,len(edgeList) - 1)])
            radiusIndex += 1
        mesh['relativeLineRadii'] = lineRadii           
                
    # face colors
    faceColorsTag = tag.find("faceAttributes/DataList[@attribute='colors']")
    if faceColorsTag is not None:
        faceColors = parseGeometryAttribute(faceColorsTag, faceAttributesSize, True, True)
        mesh['faceColors'] = faceColors
        colorLayer = mesh.vertex_colors.new(name='Face Colors')
        colorIndex = 0
        faceIndex = 0
        for poly in mesh.polygons:
            for vertexIndex in poly.vertices:
                colorLayer.data[colorIndex].color = faceColors[faceIndex]
                colorIndex += 1 
            faceIndex += 1   
            
    # texture coordinates
    textureCoordinatesTag = vertexAttributes.find("DataList[@attribute='texture coordinates']")
    if textureCoordinatesTag is not None:
        textureCoordinates = parseGeometryAttribute(textureCoordinatesTag, vertexAttributesSize, True, True)
        mesh['texture coordinates'] = textureCoordinates
        uvTexture = mesh.uv_textures.new(name='Texture Coordinates')
        uvLayer = mesh.uv_layers['Texture Coordinates']
        coordIndex = 0
        for poly in mesh.polygons:
            for vertexIndex in poly.vertices:
                uvLayer.data[coordIndex].uv = textureCoordinates[vertexIndex][0:2]
                coordIndex += 1
                    
    return mesh


def parseCustomMaterialAttribute(tag, attribute, parentMaterial, type):
    attributeTag = tag.find("attribute[@name='" + attribute + "']")
    if attributeTag is not None:
        if type == 'BOOL':
            return spheresDrawTag.find('boolean').text == true
        else:
            spheresDrawTag.find('boolean').text
    else:
        return parentMaterial[attribute]


def createTexture(tag, baseName):
    imageWidth = int(tag.get('width'))
    imageHeight = int(tag.get('height'))
    imageString = base64.b64decode(tag.text)
    imageDataByte = zlib.decompress(imageString)
    imageDataFloat = [b/255.0 for b in imageDataByte]
    texture = bpy.data.textures.new(name=baseName + ' Texture', type='IMAGE')
    image = bpy.data.images.new(name=baseName + ' Image', width=imageWidth, height=imageHeight, alpha=True, float_buffer=False)
    image.pixels = imageDataFloat
    image.pack(as_png=True)
    texture.image = image
    return texture


def createMaterial(treeRoot, tag, rootPath, parentMaterial, geometryObject):
    tag = resolveReference(treeRoot, tag, rootPath);
    if tag in tagToMaterial: 
    	return tagToMaterial[tag]
    nameTag = tag.find('name');
    mesh = None if geometryObject is None else geometryObject.data
    vertex_colors = None if mesh is None else mesh.vertex_colors if type(mesh) == bpy.types.Mesh else None
    if nameTag is None:
        if geometryObject is None or not vertex_colors:
            return None
        else:
            name = "Vertex Paint Material"
    else: 
        name = nameTag.text
    material = parentMaterial.copy()
    tagToMaterial[tag] = material
    material.name = name
    material.use_vertex_color_paint = bool(vertex_colors)
    
    # diffuse color
    diffuseColorTag = tag.find("attribute[@name='polygonShader.diffuseColor']")
    if diffuseColorTag is not None: 
        material.diffuse_color = parseColor(treeRoot, diffuseColorTag.find('color'), rootPath + "/attribute[@name='polygonShader.diffuseColor']/color")
    else: 
        material.diffuse_color = parentMaterial.diffuse_color
    diffuseFactorTag = tag.find("attribute[@name='polygonShader.diffuseCoefficient']")
    if diffuseFactorTag is not None: 
        material.diffuse_intensity = float(diffuseFactorTag.find('double').text)
    else: 
        material.diffuse_intensity = parentMaterial.diffuse_intensity
        
    # smooth/flat shading
    smoothShadingTag = tag.find("attribute[@name='polygonShader.smoothShading']")
    if smoothShadingTag is not None:
        smoothShading = smoothShadingTag.find('boolean').text == 'true'
    else:
        smoothShading = parentMaterial['polygonShader.smoothShading']
    material['polygonShader.smoothShading'] = smoothShading
    
    # choose vertex color channel   
    if vertex_colors is not None:    
        # TODO: does not work correctly with shared geometry
        if 'Vertex Colors' in vertex_colors:
            vertex_colors['Vertex Colors'].active = smoothShading
            vertex_colors['Vertex Colors'].active_render = smoothShading
        if 'Face Colors' in vertex_colors:
            vertex_colors['Face Colors'].active = not smoothShading
            vertex_colors['Face Colors'].active_render = not smoothShading
            
    # point visibility        
    showPointsTag = tag.find("attribute[@name='showPoints']")
    if showPointsTag is not None:
        showPoints = showPointsTag.find('boolean').text == 'true'
    else:
        showPoints = parentMaterial['showPoints']
    material['showPoints'] = showPoints
    
    # edge visibility        
    showLinesTag = tag.find("attribute[@name='showLines']")
    if showLinesTag is not None:
        showLines = showLinesTag.find('boolean').text == 'true'
    else:
        showLines = parentMaterial['showLines']
    material['showLines'] = showLines
    
    # face visibility        
    showFacesTag = tag.find("attribute[@name='showFaces']")
    if showFacesTag is not None:
        showFaces = showFacesTag.find('boolean').text == 'true'
    else:
        showFaces = parentMaterial['showFaces']
    material['showFaces'] = showFaces

    # draw spheres        
    spheresDrawTag = tag.find("attribute[@name='pointShader.spheresDraw']")
    if spheresDrawTag is None:
        spheresDrawTag = tag.find("attribute[@name='spheresDraw']")
    if spheresDrawTag is not None:
        spheresDraw = spheresDrawTag.find('boolean').text == 'true'
    else:
        spheresDraw = parentMaterial['pointShader.spheresDraw']
    material['pointShader.spheresDraw'] = spheresDraw

    # draw tubes
    tubesDrawTag = tag.find("attribute[@name='lineShader.tubeDraw']")
    if tubesDrawTag is None:
        tubesDrawTag = tag.find("attribute[@name='tubeDraw']")
    if tubesDrawTag is not None:
        tubesDraw = tubesDrawTag.find('boolean').text == 'true'
    else:
        tubesDraw = parentMaterial['lineShader.tubeDraw']
    material['lineShader.tubeDraw'] = tubesDraw 
     
    # sphere radius
    pointRadiusTag = tag.find("attribute[@name='pointShader.pointRadius']")
    if pointRadiusTag is None:
        pointRadiusTag = tag.find("attribute[@name='pointRadius']")
    if pointRadiusTag is not None:
        pointRadius = float(pointRadiusTag.find('double').text)
    else:
        pointRadius = parentMaterial['pointShader.pointRadius']
    material['pointShader.pointRadius'] = pointRadius
    
    # spheres radii world coordinates   
    sphereRadiiWorldCoordsTag = tag.find("attribute[@name='pointShader.radiiWorldCoordinates']")
    if sphereRadiiWorldCoordsTag is not None:
        sphereRadiiWorldCoords = sphereRadiiWorldCoordsTag.find('boolean').text == 'true'
    else:
        sphereRadiiWorldCoords = parentMaterial['pointShader.radiiWorldCoordinates']
    material['pointShader.radiiWorldCoordinates'] = sphereRadiiWorldCoords  
       
    # tube radius   
    tubeRadiusTag = tag.find("attribute[@name='lineShader.tubeRadius']")
    if tubeRadiusTag is None:
        tubeRadiusTag = tag.find("attribute[@name='tubeRadius']")
    if tubeRadiusTag is not None:
        tubeRadius = float(tubeRadiusTag.find('double').text)
    else:
        tubeRadius = parentMaterial['lineShader.tubeRadius']
    material['lineShader.tubeRadius'] = tubeRadius
    
    # tubes radii world coordinates   
    tubeRadiiWorldCoordsTag = tag.find("attribute[@name='lineShader.radiiWorldCoordinates']")
    if tubeRadiiWorldCoordsTag is not None:
        tubeRadiiiWorldCoords = tubeRadiiWorldCoordsTag.find('boolean').text == 'true'
    else:
        tubeRadiiiWorldCoords = parentMaterial['lineShader.radiiWorldCoordinates']
    material['lineShader.radiiWorldCoordinates'] = tubeRadiiiWorldCoords  
    
    # generic world coordinates
    radiiWorldCoordinatesTag = tag.find("attribute[@name='radiiWorldCoordinates']")
    if radiiWorldCoordinatesTag is not None:
        radiiWorldCoordinates = radiiWorldCoordinatesTag.find('boolean').text == 'true'
    else:
        radiiWorldCoordinates = parentMaterial['radiiWorldCoordinates']
    material['radiiWorldCoordinates'] = radiiWorldCoordinates
      
    # line colors
    lineColorTag = tag.find("attribute[@name='lineShader.diffuseColor']")
    if lineColorTag is not None:
        tubeColor = parseColor(treeRoot, lineColorTag.find('color'), rootPath + "/attribute[@name='lineShader.diffuseColor']/color")
    else:
        tubeColor = parentMaterial['lineShader.diffuseColor']
    material['lineShader.diffuseColor'] = tubeColor
    
    # point colors
    pointColorTag = tag.find("attribute[@name='pointShader.diffuseColor']")
    if pointColorTag is not None:
        sphereColor = parseColor(treeRoot, pointColorTag.find('color'), rootPath + "/attribute[@name='pointShader.diffuseColor']/color")
    else:
        sphereColor = parentMaterial['pointShader.diffuseColor']
    material['pointShader.diffuseColor'] = sphereColor 
    
    # transparency
    faceTransparencyTag = tag.find("attribute[@name='polygonShader.transparency']")
    if faceTransparencyTag is None:
        faceTransparencyTag = tag.find("attribute[@name='transparency']")
    if faceTransparencyTag is not None:
        faceTransparency = float(faceTransparencyTag.find('double').text)
    else:
        faceTransparency = 1 - parentMaterial.alpha
    transparencyEnabledTag = tag.find("attribute[@name='transparencyEnabled']")
    if transparencyEnabledTag is not None:
        transparencyEnabled = transparencyEnabledTag.find('boolean').text == 'true'
    else:
        transparencyEnabled = parentMaterial.use_transparency
    opaqueTubesAndSpheresTag = tag.find("attribute[@name='opaqueTubesAndSpheres']")
    if opaqueTubesAndSpheresTag is not None:
        opaqueTubesAndSpheres = opaqueTubesAndSpheresTag.find('boolean').text == 'true'
    else:
        opaqueTubesAndSpheres = parentMaterial['opaqueTubesAndSpheres']    
    material.alpha = 1 - faceTransparency
    material.use_transparency = transparencyEnabled
    material.transparency_method = 'RAYTRACE'
    material['opaqueTubesAndSpheres'] = opaqueTubesAndSpheres
    
    # global background color
    backgroundColorTag = tag.find("attribute[@name='backgroundColor']")
    if backgroundColorTag is not None:
        backgroundColor = parseColor(treeRoot, backgroundColorTag.find('color'), rootPath + "/attribute[@name='backgroundColor']/color")
        bpy.context.scene.world.horizon_color = backgroundColor
        
    # texture
    texture2DTag = tag.find("attribute[@name='polygonShader.texture2d']")
    textureImageTag = tag.find("attribute[@name='polygonShader.texture2d:image']")
    if texture2DTag is not None and textureImageTag is not None:
        imageDataTag = textureImageTag.find('ImageData')
        imageDataTag = resolveReference(treeRoot, imageDataTag, rootPath + "/attribute[@name='polygonShader.texture2d:image']/ImageData")
        if imageDataTag in tagToTexture:
            texture = tagToTexture[imageDataTag]
        else:
            texture = createTexture(imageDataTag, name)
            tagToTexture[imageDataTag] = texture
        material.texture_slots[0].texture = texture
        material.texture_slots[0].texture_coords = 'UV'
        material.texture_slots[0].uv_layer = 'Texture Coordinates'
       
    # texture matrix
    textureMatrixTag = tag.find("attribute[@name='polygonShader.texture2d:textureMatrix']") 
    if textureMatrixTag is not None:
        textureMatrix = parseMatrix(textureMatrixTag.find('Matrix'))
    else:
        textureMatrix = parentMaterial['polygonShader.texture2d:textureMatrix']      
    material['polygonShader.texture2d:textureMatrix'] = textureMatrix   
       
    # skin tubes
    useSkinTubesTag = tag.find("attribute[@name='lineShader.blender.useSkinTubes']")  
    if useSkinTubesTag is not None:
        useSkinTubes = useSkinTubesTag.find('boolean').text == 'true'
        material['lineShader.blender.useSkinTubes'] = useSkinTubes
    else:
        material['lineShader.blender.useSkinTubes'] = parentMaterial['lineShader.blender.useSkinTubes']
        
    return material


def createGeometry(treeRoot, tag, rootPath, parentObject):
    tag = resolveReference(treeRoot, tag, rootPath);
    name = tag.find('name');
    if name == None: return None
    type = tag.get('type')
    geom = None
    if tag in tagToObject: 
        geom = tagToObject[tag].data
    else:
        if type == 'IndexedFaceSet' or type == 'IndexedLineSet' or type == 'PointSet':
            geom = createMesh(tag)
        elif type == 'Sphere':
            geom = getStandardSphere()
        elif type == 'Cylinder':
            geom = getStandardCylinder()
        # add dummy material to create slot on object
        geom.materials.append(None)
    geomobj = bpy.data.objects.new(name=name.text, object_data = geom)
    bpy.context.scene.objects.link(geomobj)
    geomobj.parent = parentObject
    geomobj.hide = parentObject.hide
    geomobj.hide_render = parentObject.hide_render
    tagToObject[tag] = geomobj
    return geomobj


def createCamera(treeRoot, tag, rootPath, parentObject):
    tag = resolveReference(treeRoot, tag, rootPath);
    if tag.find('name') == None: return None
    name = tag.find('name').text
    if tag in tagToObject :
        cam = tagToObject[tag].data
    else :
        cam = bpy.data.cameras.new(name)
        cam.clip_start = float(tag.find('near').text);
        cam.clip_end = float(tag.find('far').text);
        cam.angle = 2.0 * math.radians(float(tag.find('fieldOfView').text));
        cam.ortho_scale = float(tag.find('focus').text);
        if tag.find('perspective').text == 'false':
            cam.type = 'ORTHO'
    camobj = bpy.data.objects.new(name=name, object_data = cam)
    trafo = tag.find('orientationMatrix')
    if trafo.text != None:
        camobj.matrix_local = parseMatrix(trafo)
    camobj.parent = parentObject
    bpy.context.scene.objects.link(camobj)
    tagToObject[tag] = camobj
    return camobj


def createLight(treeRoot, tag, rootPath, parentObject):
    tag = resolveReference(treeRoot, tag, rootPath);
    if tag.find('name') == None: return None
    name = tag.find('name').text
    type = tag.get('type')
    if tag in tagToObject :
        light = tagToObject[tag].data
    else :
        blenderType = 'POINT'
        if type == 'PointLight': 
            light = bpy.data.lamps.new(name, 'POINT')
        elif type == 'SpotLight': 
            light = bpy.data.lamps.new(name, 'SPOT')
            light.spot_size = float(tag.find('coneAngle').text)
            light.show_cone = True
        elif type == 'DirectionalLight':
            light = bpy.data.lamps.new(name, 'HEMI')     
        else:
            light = bpy.data.lamps.new(name, 'POINT')
        light.color = parseColor(treeRoot, tag.find('color'), rootPath + '/color')
        light.energy = float(tag.find('intensity').text)
    lightobj = bpy.data.objects.new(name=name, object_data = light)
    lightobj.parent = parentObject
    bpy.context.scene.objects.link(lightobj)
    tagToObject[tag] = lightobj
    return lightobj


def createSphereMaterial(mesh, index, parentMaterial):
    if 'vertexColors' in mesh:
        material = parentMaterial.copy()
        material.name = parentMaterial.name + ' Sphere Color'
        material.diffuse_color = mesh['vertexColors'][index]
    else: 
        if parentMaterial.name in sphereMaterials:
            return sphereMaterials[parentMaterial.name]
        else:
            material = parentMaterial.copy()
            material.name = parentMaterial.name + ' Spheres'
            material.diffuse_color = material['pointShader.diffuseColor']
            sphereMaterials[parentMaterial.name] = material
    material.use_transparency = not parentMaterial['opaqueTubesAndSpheres']
    material.use_vertex_color_paint = False
    material.texture_slots[0].texture = None
    return material


def createTubeMaterial(mesh, index, parentMaterial):
    if 'edgeColors' in mesh:
        material = parentMaterial.copy()
        material.name = parentMaterial.name + ' Tube Color'
        material.diffuse_color = mesh['edgeColors'][index]
    else:
        if parentMaterial.name in tubeMaterials:
            return tubeMaterials[parentMaterial.name]
        else:
            material = parentMaterial.copy()
            material.name = parentMaterial.name + ' Tubes'
            material.diffuse_color = material['lineShader.diffuseColor']
            tubeMaterials[parentMaterial.name] = material
    material.use_transparency = not parentMaterial['opaqueTubesAndSpheres']
    material.use_vertex_color_paint = False
    material.texture_slots[0].texture = None
    return material


def getWorldScale():
    T = transformStack[-1]
    scale = T.decompose()[2][0]
    return scale


def createTubesAndSpheres(geometryObject, material):
    mesh = geometryObject.data
    sphereRadiiWorldCoordinates = material['pointShader.radiiWorldCoordinates']
    tubeRadiiWorldCoordinates = material['lineShader.radiiWorldCoordinates']
    if material['radiiWorldCoordinates']:
        sphereRadiiWorldCoordinates = True
        tubeRadiiWorldCoordinates = True
    if material['pointShader.spheresDraw'] and material['showPoints'] and type(mesh) == bpy.types.Mesh and mesh.vertices:
        # TODO: respect radii world coordinates flag here and for tubes
        sphereRadius = material['pointShader.pointRadius']
        if sphereRadiiWorldCoordinates:
            sphereRadius /= getWorldScale()
        spheresObject = bpy.data.objects.new(name='Vertex Spheres', object_data=None)
        spheresObject.parent = geometryObject
        spheresObject.hide = geometryObject.hide
        spheresObject.hide_render = geometryObject.hide_render 
        bpy.context.scene.objects.link(spheresObject)
        sphereGeometry = createNURBSSphereData('NURBS Sphere')
        for v in mesh.vertices:
            radius = sphereRadius
            if 'relativePointRadii' in mesh:
                radius *= mesh['relativePointRadii'][v.index][0]
            sphereObject = bpy.data.objects.new(name='Sphere', object_data=sphereGeometry)
            sphereObject.parent = spheresObject
            sphereObject.hide = geometryObject.hide
            sphereObject.hide_render = geometryObject.hide_render
            sphereObject.location = v.co
            sphereObject.scale = [radius, radius, radius]
            bpy.context.scene.objects.link(sphereObject)
            mat = createSphereMaterial(mesh, v.index, material)
            sphereObject.material_slots[0].link = 'OBJECT'
            sphereObject.material_slots[0].material = mat
    if not material['lineShader.blender.useSkinTubes'] and \
           material['lineShader.tubeDraw'] and \
           material['showLines'] and \
           type(mesh) == bpy.types.Mesh and \
           mesh.edges:
        tubeRadius = material['lineShader.tubeRadius']
        if tubeRadiiWorldCoordinates:
            tubeRadius /= getWorldScale()
        tubesObject = bpy.data.objects.new(name='Edge Tubes', object_data=None)
        tubesObject.parent = geometryObject
        tubesObject.hide = geometryObject.hide
        tubesObject.hide_render = geometryObject.hide_render 
        bpy.context.scene.objects.link(tubesObject)
        tubeGeometry = createNURBSCylinderData('NURBS Cylinder')
        for e in mesh.edges:
            radius = tubeRadius
            if 'relativeLineRadii' in mesh:
                radius *= mesh['relativeLineRadii'][e.index][0]
            v0 = mesh.vertices[e.vertices[0]].co
            v1 = mesh.vertices[e.vertices[1]].co  
            mid = (v0 + v1) / 2
            length = (v0 - v1).length
            targetZ = (v0 - v1).normalized()
            seed = Vector([[1, 0, 0],[0, 1, 0]][targetZ[0] != 0])
            targetX = targetZ.cross(seed).normalized()
            targetY = targetX.cross(targetZ)
            T = Matrix([targetX, targetY, targetZ])
            T.transpose()
            T.resize_4x4()
            T[0][3] = mid[0] 
            T[1][3] = mid[1]
            T[2][3] = mid[2]
            tubeObject = bpy.data.objects.new(name='Tube', object_data=tubeGeometry)
            tubeObject.parent = tubesObject
            tubeObject.hide = geometryObject.hide
            tubeObject.hide_render = geometryObject.hide_render
            tubeObject.matrix_local = T
            tubeObject.scale = [radius, radius, length/2]
            bpy.context.scene.objects.link(tubeObject)
            mat = createTubeMaterial(mesh, e.index, material)
            tubeObject.material_slots[0].link = 'OBJECT'
            tubeObject.material_slots[0].material = mat

    
def createSkinTubes(geometryObject, material):
    mesh = geometryObject.data
    skin = geometryObject.modifiers.new('Skin', 'SKIN')
    skin.use_smooth_shade = True
    subsurf = geometryObject.modifiers.new('Subdivision', 'SUBSURF')
    subsurf.levels = 2
    subsurf.render_levels = 3
    tubeRadius = material['lineShader.tubeRadius']
    if material['lineShader.radiiWorldCoordinates'] or material['radiiWorldCoordinates']:
        tubeRadius /= getWorldScale()
    for v in mesh.vertices:
        radius = tubeRadius
        if 'relativePointRadii' in mesh:
            radius *= mesh['relativePointRadii'][v.index][0]
        mesh.skin_vertices[0].data[v.index].radius = [radius, radius]
    material.diffuse_color = material['lineShader.diffuseColor']
    
    
def applyTextureMatrix(mesh, material):
    T = Matrix(material['polygonShader.texture2d:textureMatrix'])
    for uv in mesh.uv_layers['Texture Coordinates'].data:
        uv2 = T * Vector([uv.uv[0], uv.uv[1], 0, 1])
        uv.uv = [uv2[0] / uv2[3], uv2[1] / uv2[3]]
    
    
def createObjectFromXML(treeRoot, tag, rootPath, parentObject, visible):
    originalTag = tag
    tag = resolveReference(treeRoot, tag, rootPath);
    if originalTag != tag:
        rootPath = resolveReferencePath(treeRoot, originalTag, rootPath);
    name = tag[0].text
    obj = bpy.data.objects.new(name, None)
    if not visible:
        obj.hide = True
        obj.hide_render = obj.hide
    else: 
        visible = tag.find('visible').text == 'true'
        obj.hide = not visible
        obj.hide_render = obj.hide
    trafo = tag.find('transformation/matrix')
    if trafo != None:
        obj.matrix_local = parseMatrix(trafo)
    transformStack.append(transformStack[-1] * obj.matrix_local)   
    bpy.context.scene.objects.link(obj)
    if parentObject != None :
        obj.parent = parentObject
    camera = createCamera(treeRoot, tag.find('camera'), rootPath + '/camera', obj)
    geometry = createGeometry(treeRoot, tag.find('geometry'), rootPath + '/geometry', obj);
    light = createLight(treeRoot, tag.find('light'), rootPath + '/light', obj);
    material = createMaterial(treeRoot, tag.find('appearance'), rootPath + '/appearance', materialStack[-1], geometry);
    if material is not None: 
        materialStack.append(material)
    if geometry is not None:
        effectiveMaterial = materialStack[-1]
        useSkinTubes = effectiveMaterial['lineShader.blender.useSkinTubes']
        tubeDraw = effectiveMaterial['lineShader.tubeDraw']
        createTubesAndSpheres(geometry, effectiveMaterial)
        if tubeDraw and useSkinTubes and geometry.data is not None:
            createSkinTubes(geometry, effectiveMaterial)
        showFaces = bool(effectiveMaterial['showFaces'])
        geometry.hide = obj.hide or not (showFaces or useSkinTubes)
        geometry.hide_render = geometry.hide
        if type(geometry.data) == bpy.types.Mesh and 'Texture Coordinates' in geometry.data.uv_layers:
            applyTextureMatrix(geometry.data, effectiveMaterial)
        geometry.material_slots[0].link = 'OBJECT'
        geometry.material_slots[0].material = effectiveMaterial
    counter = 1;
    for child in tag.find("./children"):
        path = rootPath + '/children/child[' + str(counter) + ']'
        counter += 1
        createObjectFromXML(treeRoot, child, path, obj, visible);
    if material is not None: materialStack.pop() 
    transformStack.pop() 
    return obj    


def resolveReference(treeRoot, tag, rootPath):
    if 'reference' in tag.attrib :
        refPath = rootPath + '/' + tag.attrib['reference']
        return treeRoot.find(refPath)
    return tag

def resolveReferencePath(treeRoot, tag, rootPath):
    if 'reference' in tag.attrib :
        refPath = rootPath + '/' + tag.attrib['reference']
        return refPath
    return rootPath
        
def createDefaultMaterial():
    mtl = bpy.data.materials[0]
    mtl.name = 'JReality Default Material'
    mtl.diffuse_color = [0, 0, 1]
    mtl.diffuse_intensity = 1.0
    mtl.alpha = 1.0
    mtl['showPoints'] = True
    mtl['showLines'] = True
    mtl['showFaces'] = True
    mtl['radiiWorldCoordinates'] = False
    mtl['polygonShader.smoothShading'] = True
    mtl['polygonShader.texture2d:textureMatrix'] = Matrix()
    mtl['pointShader.spheresDraw'] = True
    mtl['pointShader.diffuseColor'] = [0.0, 0.0, 1.0]
    mtl['pointShader.radiiWorldCoordinates'] = False
    mtl['pointShader.pointRadius'] = 0.025
    mtl['lineShader.tubeDraw'] = True
    mtl['lineShader.tubeRadius'] = 0.025
    mtl['lineShader.diffuseColor'] = [0.0, 0.0, 1.0]
    mtl['lineShader.radiiWorldCoordinates'] = False
    mtl['lineShader.blender.useSkinTubes'] = False
    mtl['opaqueTubesAndSpheres'] = False
    return mtl
        
        
def createSceneFromXML(scene_file):
    # parse xml
    sceneTree = ET.parse(scene_file)
    root = sceneTree.getroot()
    # traverse scene xml
    materialStack.append(createDefaultMaterial())
    transformStack.append(Matrix.Identity(4))
    rootObject = createObjectFromXML(root, root[0], './sceneRoot', None, True)
    # create coordinate conversion root
    sceneObject = bpy.data.objects.new('JReality Scene', None)
    jrealityToBlender = Euler((math.pi/2, 0.0, math.pi/2), 'XYZ')
    sceneObject.matrix_local = jrealityToBlender.to_matrix().to_4x4()
    bpy.context.scene.objects.link(sceneObject)
    rootObject.parent = sceneObject;
    # find active camera
    cameraPath = root.find("scenePaths/path[@name='cameraPath']")
    if cameraPath != None:
        cameraPathXpath = resolveReferencePath(root, cameraPath, "./scenePaths/path[@name='cameraPath']")
        cameraPath = resolveReference(root, cameraPath, "./scenePaths/path[@name='cameraPath']")
        node = cameraPath.find('node[last()]')
        camTag = resolveReference(root, node, cameraPathXpath + "/node[last()]")
        bpy.context.scene.camera = tagToObject[camTag]
    else:
        print('WARNING: no camera path set')
        

def readJRealityScene(scene_file, save_path, render_path):
    scene = bpy.context.scene
    # Clear existing objects.
    scene.camera = None
    for obj in scene.objects:
        scene.objects.unlink(obj)
    createSceneFromXML(scene_file)
    if save_path:
        try:
            f = open(save_path, 'w')
            f.close()
            ok = True
        except:
            print("Cannot save to path %r" % save_path)
            
            import traceback
            traceback.print_exc()
        
        if ok:
            bpy.ops.wm.save_as_mainfile(filepath=save_path)
    if render_path:
        render = scene.render
        render.use_file_extension = True
        render.filepath = render_path
        bpy.ops.render.render(write_still=True)


def checkBlenderVersion():
    version = bpy.app.version
    return version[0] == 2 and version[1] >= 70

def main():
    import sys
    import argparse
    argv = sys.argv
    if "--" not in argv:
        argv = []
    else:
        argv = argv[argv.index("--") + 1:] 
    usage_text = \
    "Run blender in background mode with this script:"
    "  blender --background --python " + __file__ + " -- [options]"
    parser = argparse.ArgumentParser(description=usage_text)
    parser.add_argument("-s", "--save", dest="save_path", metavar='FILE', help="Save the generated file to the specified path")
    parser.add_argument("-r", "--render", dest="render_path", metavar='FILE', help="Render an image to the specified path")
    parser.add_argument("-f", "--file", dest="scene_path", metavar='FILE', help="Render the specified scene")
    args = parser.parse_args(argv)  # In this example we wont use the args
    if not argv:
        parser.print_help()
        return
    
    if not checkBlenderVersion():
        sys.stderr.write('JReality blender export needs blender version 2.70 or newer')
        return
    readJRealityScene(args.scene_path, args.save_path, args.render_path)


if __name__ == "__main__":
    main()
