package de.jreality.writer.u3d;


public final class U3DConstants {

	public final static short
		EXPORTER_VERSION_MINOR = 0,
		EXPORTER_VERSION_MAJOR = 0;
	public final static long 
		PROFILE_OPTIONS = 0x00000000,
		CHARACTER_ENCODING = 106,
		TYPE_FILE_HEADER = 0x00443355,
		TYPE_FILE_REFERENCE = 0xFFFFFF12,
		TYPE_MOFIFIER_CHAIN = 0xFFFFFF14,
		TYPE_PRIORITY_UPDATE = 0xFFFFFF15,
		
		TYPE_GROUP_NODE = 0xFFFFFF21,
		TYPE_MODEL_NODE = 0xFFFFFF22,
		TYPE_LIGHT_NODE = 0xFFFFFF23,
		TYPE_VIEW_NODE = 0xFFFFFF24,
	
		TYPE_MESH_GENERATOR_DECLARATION = 0xFFFFFF31,
		TYPE_MESH_GENERATOR_CONTINUATION = 0xFFFFFF3B,
		TYPE_MESH_GENERATOR_PROGRESSIVE_CONTINUATION = 0xFFFFFF3C,
		
		TYPE_POINT_SET_DECLARATION = 0xFFFFFF36,
		TYPE_POINT_SET_CONTINUATION = 0xFFFFFF3E,
		
		TYPE_LINE_SET_DECLARATION = 0xFFFFFF73,
		TYPE_LINE_SET_CONTINUATION = 0xFFFFFF3F,
		
		TYPE_SHADING_MODIFIER = 0xFFFFFF45,
		
		TYPE_LIGHT_RESOURCE = 0xFFFFFF51,
		TYPE_VIEW_RESOURCE = 0xFFFFFF52,
		TYPE_LIT_TEXTURE_SHADER = 0xFFFFFF53,
		TYPE_MATERIAL_RESOURCE = 0xFFFFFF54,
		TYPE_TEXTURE_RESOURCE_DECLARATION = 0xFFFFFF55,
		TYPE_TEXTURE_RESOURCE_CONTINUATION = 0xFFFFFF5C;
	
	
	public final static float
		fLimit = (float) 0x00feffff;
	
	public static final int 
		uACStaticFull = 0x00000400,
		uACMaxRange = uACStaticFull + 0x00003FFF;

	// BlockType_MotionCompressed = 0xFFFFFF64;
	public static final int 
		uACContextMotionSignTime = 1,
		uACContextMotionDiffTime = 2,
		uACContextMotionSignDisplacement = 3,
		uACContextMotionDiffDisplacement = 4,
		uACContextMotionSignRotation = 5,
		uACContextMotionDiffRotation = 6,
		uACContextMotionSignScale = 7,
		uACContextMotionDiffScale = 8;

	// BlockType_AuthorCLODStatic
	public static final int 
		uACContextBaseShadingID = 1;

	// BlockType_AuthorCLODProgressive
	/// @todo: Renumber contexts
	public static final byte 
		u8AMPOrientationLeft = 1,
		u8AMPOrientationRight = 2,
		u8AMPThirdIndexLocal = 1,
		u8AMPThirdIndexGlobal = 2;

	public static final int 
		uACContextNumNewFaces = 1,
		uACContextNumNewTexCoords   = 64;

	public static final int 
		uACContextShadingID = 65; // Todo: Reorder and renumber contexts

	public static final int 
		uACContextOrientationType = 2,
		uACContextThirdIndexType = 3,
		uACContextLocal3rdPosition = 4;

	public static final int 
		uAMPPredictStay2 = 4,
		uAMPPredictMove2 = 3,
		uAMPPredictStay = 2,
		uAMPPredictMove = 1,
		uAMPPredictNoGuess = 0;
	public static final byte 
		u8AMPStay = 0,
		u8AMPMove = 1;
	public static final int 
		uACContextStayMove = 15,
		// reserved for Stay/Move:
		// [uACContextStayMove ... uACContextStayMove + uAMPPredictStay2]
		// currently that is [15 ... 19]
		uACContextPositionDiffSigns = 20,
		uACContextPositionDiffMagX = 21,
		uACContextPositionDiffMagY = 22,
		uACContextPositionDiffMagZ = 23,
	
		uACContextNewDiffuseColorsCount = 99,
		uACContextDiffuseColorSign = 100,
		uACContextNewSpecularColorsCount = 101,
		uACContextSpecularColorSign = 102,
		uACContextTexCoordSign = 103;

	public static final byte 
		uAMPUpdateChange = 1,
		uAMPUpdateKeep = 2,
		uAMPUpdateNew = 1,
		uAMPUpdateLocal = 2,
		uAMPUpdateGlobal = 3;

	public static final int 
		uACContextDiffuseKeepChange = 104,
		uACContextDiffuseChangeType = 105,
		uACContextDiffuseChangeIndexNew = 106,
		uACContextDiffuseChangeIndexLocal = 107,
		uACContextDiffuseChangeIndexGlobal = 108,
	
		uACContextSpecularKeepChange = 109,
		uACContextSpecularChangeType = 110,
		uACContextSpecularChangeIndexNew = 111,
		uACContextSpecularChangeIndexLocal = 112,
		uACContextSpecularChangeIndexGlobal = 113,
	
		uACContextTexCoordKeepChange = 114,
		uACContextTexCoordChangeType = 115,
		uACContextTexCoordChangeIndexNew = 116,
		uACContextTexCoordChangeIndexLocal = 117,
		uACContextTexCoordChangeIndexGlobal = 118,
	
		uACContextVertColorSplitIndexLocal = 119,
		uACContextVertColorSplitIndexGlobal = 120,
	
		uACContextTexCoordSplitIndexLocal = 121,
		uACContextTexCoordSplitIndexGlobal = 122,
		uACContextNewTexCoordsCount = 123,
	
		uACContextTexCoordUsedType  = 24,
		uACContextTexCoordIndexType = 25,
		uACContextTexCoordCandType  = 26,
		uACContextTexCoordListType  = 27,
		//uACContextTexCoordChangeType= 28,
		uACContextTexCoordSplitType = 29,
		uACContextCandidateIndex    = 30,
		uACContextListIndex     = 31,
		uACContextTexCoordDiffSigns = 32,
		uACContextTexCoordDiffMagU  = 33,
		uACContextTexCoordDiffMagV  = 34,
		uACContextTexCoordDiffMagS  = 35,
		uACContextTexCoordDiffMagT  = 36,
		uACContextTexCoordAttribType = 37,
		uACContextTexCoordFaceUpdate = 38,
		uACContextTexCoordDupType    = 39;

	public static final byte  
		u8AMPOldTexCoord = 0,
		u8AMPNewTexCoord = 1,
		u8AMPRawTexCoord = 5,
		u8AMPCurTexCoord = 0, //Indicates to use the current TexCoords.
		u8AMPSplitTexCoord = 1,
		u8AMPThirdTexCoord = 2,
		u8AMPOneLessTexCoord=3,
		u8AMPTwoLessTexCoord=4,
		u8AMPCandidate   = 0,
		u8AMPNotCandidate  = 1,
		u8AMPNotList   = 0,
		u8AMPList      = 1,
		u8AMPKeep      = 0,
		u8AMPChange    = 1,
		u8AMPGood      = 0,
		u8AMPBad     = 1,
		u8AMPSplitTexCoordDup  = 1,
		u8AMPUpdateTexCoordDup = 2,
		u8AMPThirdTexCoordDup  = 4;

	public static final int 
		uACContextNumLocalNormals    = 40,
		uACContextNormalDiffSigns    = 41,
		uACContextNormalDiffMagX   = 42,
		uACContextNormalDiffMagY   = 43,
		uACContextNormalDiffMagZ   = 44,
		uACContextNormalLocalIndex   = 45,
		uACContextNumNewDiffuseColors  = 46,
		uACContextNumNewSpecularColors = 47,
	
		uACContextVertColorCount   = 50,
		uACContextVertColorUsedType  = 51,
		uACContextVertColorCandType  = 52,
		uACContextVertColorListType  = 53,
		uACContextVertColorChangeType  = 54,
		uACContextVertColorSplitType = 55,
		uACContextVertColorDupType   = 56,
		uACContextVertexColorDiffMagR  = 60,
		uACContextVertexColorDiffMagG  = 61,
		uACContextVertexColorDiffMagB  = 62,
		uACContextVertexColorDiffMagA  = 63,
	
		u8AMPOldVertColor   = 0,
		u8AMPNewVertColor   = 1,
		u8AMPCurVertColor   = 2, //Indicates to use the current TexCoords.
		u8AMPSplitVertColor = 3,
		u8AMPThirdVertColor = 4,
		u8AMPSplitColorDup  = 1,
		u8AMPUpdateColorDup = 2,
		u8AMPThirdColorDup  = 4;

	public static final int 
		uACContextBoneWeightCount = 66,
		uACContextBoneWeightBoneID = 67,
		uACContextBoneWeightBoneWeight = 68;

	// BlockType_LineSet
	public static final int 
		uACContextLineShadingID = 1;

	public static final int 
		uACContextPointShadingID = 1;

}
