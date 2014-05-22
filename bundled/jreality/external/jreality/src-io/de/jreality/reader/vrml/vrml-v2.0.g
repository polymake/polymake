//**************************************************
// * VRML 2.0 Parser
// * Copyright (C) 1996 Silicon Graphics, Inc.
// *
// * Author(s)	: Gavin Bell
// *                Daniel Woods (first port)
// **************************************************
// */

header {
package de.jreality.reader.vrml;
import java.awt.Color;
import java.util.*;
}

//options {
//	language="java";
//}

/*****************************************************************************
 * The VRML Parser
 *****************************************************************************
 */
 class VRMLParser extends Parser;
options {
	k = 2;							// two token lookahead
	//tokenVocabulary=VRML;			// Call its vocabulary "VRML"
	//exportVocab=VRML;
}
{
	public int[] listToIntArray(List l)		{
		int[] foo = new int[l.size()];
		int count = 0;
		Iterator iter = l.iterator();
		while (iter.hasNext()	)	{
			foo[count++] = ((Integer)iter.next()).intValue();
		}
		return foo;
	}
	public double[] listToDoubleArray(List l)		{
		double[] foo = new double[l.size()];
		int count = 0;
		Iterator iter = l.iterator();
		while (iter.hasNext()	)	{
			foo[count++] = ((Double)iter.next()).doubleValue();
		}
		return foo;
	}
}
vrmlFile:
		HEADER
		vrmlScene
	;

vrmlScene:
		statements
	;

statements:
		(statement)*
	;

statement:
		nodeStatement
	|	protoStatement
	|	routeStatement
	;

nodeStatement:
		node
	|	"DEF" nodeNameId node
	|	"USE" nodeNameId
	;

rootNodeStatement:
		node
	|	"DEF" nodeNameId node
	;

protoStatement:
		proto
	|	externproto
	;

protoStatements:
		(protoStatement)*
	;

proto:
		"PROTO" nodeTypeId  
			OPEN_BRACKET interfaceDeclarations CLOSE_BRACKET
			OPEN_BRACE protoBody CLOSE_BRACE 
	;

protoBody:
		protoStatements rootNodeStatement statements
	;

interfaceDeclarations:
		(interfaceDeclaration)*			
	;

restrictedinterfaceDeclaration:
		"eventIn" fieldType  eventInId 
	|	"eventOut" fieldType  eventOutId
	|	"field" fieldTriple 
	;

interfaceDeclaration:
		restrictedinterfaceDeclaration
	|	"exposedField" fieldTriple
	;

externproto:
		"EXTERNPROTO" nodeTypeId OPEN_BRACKET externinterfaceDeclarations CLOSE_BRACKET urlList
	;

externinterfaceDeclarations:
		(externinterfaceDeclaration)*
	;

externinterfaceDeclaration:
		"eventIn" FIELDTYPE eventInId
	|	"eventOut" FIELDTYPE eventOutId
	|	"field" FIELDTYPE fieldId
	|	"exposedField" FIELDTYPE fieldId
	;

routeStatement:
		"ROUTE" nodeNameId PERIOD eventOutId "TO" nodeNameId PERIOD eventInId
	;

urlList:
		mfstringValue
	;

/* Nodes */
node:
		nodeTypeId OPEN_BRACE nodeBody CLOSE_BRACE
	|	Script OPEN_BRACE scriptBody CLOSE_BRACE
	;

nodeBody:
		(nodeBodyElement)*
	;

scriptBody:
		(scriptBodyElement)*
	;

scriptBodyElement:
		nodeBodyElement
	|	restrictedinterfaceDeclaration
	|	"eventIn" FIELDTYPE eventInId "IS" eventInId
	|	"eventOut" FIELDTYPE eventOutId "IS" eventOutId
	|	"field" FIELDTYPE fieldId "IS" fieldId
	;

//nodeBodyElement: fieldPair
nodeBodyElement: 
        fieldId "IS" fieldId
	|	eventInId "IS" eventInId
	|	eventOutId "IS" eventOutId
	|	routeStatement
	|	protoStatement
	;

id :	
	n:ID  	{System.err.println("Id matched: "+n.getText()); }
	;

nodeNameId:
	id		
	;

nodeTypeId:
	id		
	;

fieldId:
	id		
	;

eventInId:
	id		
	;

eventOutId:
	id		
	;

fieldTriple:
	"SFColor" fieldId  sfcolorValue
    | "SFFloat" fieldId sffloatValue
    | "SFImage" fieldId sfimageValue
    | "SFInt32" fieldId sfInt32Value
    | "SFNode" fieldId sfnodeValue
    | "SFRotation" fieldId sfrotationValue
    | "SFString" fieldId sfstringValue
    | "SFBool" fieldId sfboolValue
    | "SFTime" fieldId sftimeValue
    | "SFVec2f" fieldId sfvec2fValue
    | "SFVec3f" fieldId sfvec3fValue
    | "MFColor" fieldId  mfcolorValue
    | "MFFloat" fieldId mffloatValue
    | "MFInt32" fieldId mfInt32Value
    | "MFNode" fieldId mfnodeValue
    | "MFRotation" fieldId mfrotationValue
    | "MFString" fieldId mfstringValue
    | "MFTime" fieldId mftimeValue
    | "MFVec2f" fieldId mfvec2fValue
    | "MFVec3f" fieldId mfvec3fValue
    ;


number:
	f:INT32 {System.err.print("I "+f.getText());} 
	| g:FLOAT {System.err.print("F "+g.getText()); }		
	;
	
sfboolValue:
    "TRUE"	 | "FALSE" 
	;
	
sfimageValue:
	sfInt32Values	
	;

sfnodeValue:
		nodeStatement
	|	"NULL"
	;

sfrotationValue
	:
	number  number number number	
	;

sfstringValue
	:
	STRING	
	;

sftimeValue
	:
	number
	;

mftimeValue:
		sftimeValue
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sftimeValues CLOSE_BRACKET
	;

sftimeValues:
		(sftimeValue)+
	;

sfcolorValue
	:
		number number number	
	;

sfcolorValues:
		(sfcolorValue)+
	;

mfcolorValue:
		sfcolorValue
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sfcolorValues CLOSE_BRACKET
	;

sffloatValue
	:
	number
	;

sffloatValues
	:
		(sffloatValue	 )+
	;

mffloatValue:
		sffloatValue		
	|	OPEN_BRACKET CLOSE_BRACKET	
	|	OPEN_BRACKET sffloatValues CLOSE_BRACKET
	;

sfInt32Value
	:
	    INT32	
	;

sfInt32Values
	:
		(sfInt32Value)+
	;

mfInt32Value:
		sfInt32Value
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sfInt32Values CLOSE_BRACKET
	;

mfnodeValue:
		nodeStatement
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET nodeStatements CLOSE_BRACKET
	;

nodeStatements:
		(nodeStatement)+
	;

sfrotationValues:
		(sfrotationValue)+
	;

mfrotationValue:
		sfrotationValue
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sfrotationValues CLOSE_BRACKET
	;

mfstringValue:
		sfstringValue
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sfstringValues CLOSE_BRACKET
	;

sfstringValues:
		(sfstringValue)+
	;

sfvec2fValue:
		number number
	;

sfvec2fValues:
		(sfvec2fValue)+
	;

mfvec2fValue:
		sfvec2fValue
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sfvec2fValues CLOSE_BRACKET
	;

sfvec3fValue:
		number number number
	;

sfvec3fValues:
		(sfvec3fValue)+
	;

mfvec3fValue:
		sfvec3fValue
	|	OPEN_BRACKET CLOSE_BRACKET
	|	OPEN_BRACKET sfvec3fValues CLOSE_BRACKET
	;

fieldType:
		"MFColor"
	|	"MFFloat"
	|	"MFInt32"
	|	"MFNode"
	|	"MFRotation"
	|	"MFString"
	|	"MFTime"
	|	"MFVec2f"
	|	"MFVec3f"
	|	"SFBool"
	|	"SFColor"
	|	"SFFloat"
	|	"SFImage"
	|	"SFInt32"
	|	"SFNode"
	|	"SFRotation"
	|	"SFString"
	|	"SFTime"
	|	"SFVec2f"
	|	"SFVec3f"
	;
/************************************************************************************
 * The VRML Lexer
 ************************************************************************************
 */
class VRMLLexer extends Lexer;
options {
	//tokenVocabulary=VRML;		// call the vocabulary "VRML"
//	exportVocab=VRML;
	charVocabulary = '\3'..'\377';
	k=2;
	testLiterals=false;
}
protected
HEADER1:	"#VRML V2.0 utf8";

	/* Terminal Symbols */
PERIOD:			'.';
OPEN_BRACE:		'{';
CLOSE_BRACE:	'}';
OPEN_BRACKET:	'[';
CLOSE_BRACKET:	']';

//ID:
//		(~('\60'..'\71' | '\00'..'\40' | '\42' | '\43' | '\47' | '\53'..'\56'
//		| '\133'..'\135' | '\173' | '\175' | '\177')) (~('\00'..'\40' | '\42' | '\43' | '\47' | '\54' | '\56' | '\133'..'\135'
//		| '\173' | '\175'))?
//		;

ID
options {
	paraphrase = "an identifier";
	testLiterals=true;
}
	:	('a'..'z'|'A'..'Z'|'_') (ID_LETTER)*
	;

protected 
ID_LETTER:
	('a'..'z'|'A'..'Z'|'_'|'0'..'9')
	;
//FLOAT:
//		(('+'|'-')? (( (('0'..'9')+ ('.')?) | (('0'..'9')* '.' ('0'..'9')+))
//		(('e'|'E') ('+'|'-')? ('0'..'9')+)?))
//	;

//FLOAT:	  DECIMAL_BEGIN '.' ('0'..'9')+ (('e'|'E') ('+'|'-')? ('0'..'9')+)? 
//	;

//NUMBER:	DECIMAL_BEGIN ('.' DECIMAL_BEGIN)? ;

INT_OR_FLOAT : (DECIMAL_BEGIN) {$setType(INT32);}  ( '.' DECIMAL_BEGIN (EXPONENT)?{$setType(FLOAT);} )?;

INT32:
		DECIMAL_BEGIN | ('0' ('x'|'X') ('0'..'9' | 'a'..'f' | 'A'..'F')+)
	;

protected 
DECIMAL_BEGIN:
	('+'|'-')? ('0'..'9')+
	;
	
protected
EXPONENT:
	(('e'|'E') ('+'|'-')? ('0'..'9')+) 
	;
	
	/* ".*" ... double-quotes must be \", backslashes must be \\... */
STRING:
		'"' (ESC | ~('"'|'\\'))* '"'
	;

protected
ESC:
		'\\' ('\\' | '"')
	;

protected
RESTLINE:
	 (~('\n'))* ('\n')
	 ;
	 
HEADER:	{getLine()==1}?	HEADER1 RESTLINE
	{System.err.println("Got header");}
	;
	
COMMENT:	
 	'#'  (~('\n'))* ('\n')
	{ System.err.println("Skipping comment "); $setType(Token.SKIP); }
	;

WS_:
		( ' '
		| '\t'
		| '\f'
		| ','
		// handle newlines
		|	(options {
					generateAmbigWarnings=false;
				}
		: "\r\n"	// Evil DOS
			| '\r'		// MacINTosh
			| '\n'		// Unix (the right way)
			)
		)+ { $setType(Token.SKIP); }
	;


