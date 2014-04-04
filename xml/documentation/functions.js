/*
  Copyright (c) 1997-2014
  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
  http://www.polymake.org

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version: http://www.gnu.org/licenses/gpl.txt.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
===============================================================================

This file contains the JavaScript functions used in the documentation of 
polymake.
*/

// make content appear or disappear upon clicking
function swap_content( span_id ) {
	displayType = ( document.getElementById( span_id ).style.display == 'none' ) ? 'inline' : 'none';
	styleImage = ( document.getElementById( span_id ).style.display == 'none' ) ? 'url(images/minus.png)' : 'url(images/plus.png)';
	document.getElementById( span_id ).style.display = displayType;
	var id; id=span_id.split(':')[1];
	var icon; icon='icon:'+id;
	document.getElementById( icon ).style.backgroundImage = styleImage;
}

// unfold element (and also all its ancestors)
function unfold( id ) {
	var obj; obj=document.getElementById( id );
	while (obj) {
		if (obj.tagName=='DIV' && obj.className=='foldit') {
			obj.style.display = 'inline';
			id = obj.id;
			var icon; icon='icon:'+id.split(':')[1];
			document.getElementById( icon ).style.backgroundImage='url(images/minus.png)';
		}
		obj=obj.parentNode;
	}
}

// executed upon loading 
// fold everything (if JavaScript is activated)
function start() {
	var list; list=document.getElementsByTagName("div");
	for(var i=0; i< list.length; i++){
  		var temp;
  		temp=list[i];
  		if(temp.id && temp.className=='foldit'){
			temp.style.display = 'none';
		}
	}

	// jumps to anchor if there is one
	if (location.href.split('#')[1]) {
		var anchor; anchor = location.href.split('#')[1];
		var span; span='span:'+anchor;
		unfold(span);
	}
}
