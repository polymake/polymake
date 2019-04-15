
# add line numbers on the left side of the textview used for 
# the display of a property in our property_viewer
package PropertyTextView;

use strict;
use Gtk2;
use Glib qw/TRUE FALSE/; 

my $row_number_margins = 3;

use Glib::Object::Subclass
    Gtk2::TextView::,
    signals => {
        expose_event => \&redraw_property_view,   # this signal is emitted if the property view needs to be redrawn
    },
    ;

sub INIT_INSTANCE {
    my $self = shift;
    ## ??? we need to set a random width here, otherwise there is no left pane in the view
    $self->set_border_window_size(left => 10);

    # FIXME choose a monospace font
}

sub draw_line_numbers {
    my ($self, $event) = @_;
    my $window = $self->get_window('left');   # the line numbers are in the left part of our textview

    # the top left corner of the visible area relative to the buffer
    my ($xtop, $ytop)                    = $self->window_to_buffer_coords ('left', $event->area->x, $event->area->y);       
    # height and width of the visible area
    my ($visible_width, $visible_height) = $self->window_to_buffer_coords ('left', $event->area->width, $event->area->height);   

    my ($iter, $y) = $self->get_line_at_y ($ytop);   # the second argument returns the top y coordinate, we fetch this again below


    my @row_number_coords_y = ();  # the top left coord for each row number
    my @row_numbers         = ();  # and the row number
    my $height = 0;                # the height of the current line in the next while loop

    # we fill row_number_coords_y and row_numbers until we reached the end of the property, or we leave the visible area
    while ( (!$iter->is_end) && ($visible_height >= ($y + $height)) ) {
	# this fetches height and top y coordinate of the line iter points to
        ($y, $height) = $self->get_line_yrange ($iter);
        push @row_number_coords_y, $y;
        push @row_numbers, $iter->get_line;
        $iter->forward_line;
    }

    # we have to find the maximal width of a line number for the current property
    # FIXME the last number is not a very clever choice, unless we use a fixed width font (we should!)
    my $width  = sprintf "%d", $self->get_buffer->get_line_count;  
    my $layout = $self->create_pango_layout ($width);
    my ($row_number_width, $row_number_height) = $layout->get_pixel_size;
    $layout->set_width($row_number_width);
    $layout->set_alignment('right');
    my $margin_width = $row_number_width + 2*$row_number_margins;
    $self->set_border_window_size(left => $margin_width);

    my $cur = $self->get_buffer->get_iter_at_mark ($self->get_buffer->get_insert);
    my $n_rows = scalar(@row_numbers);
    for ( my $i = 0; $i < $n_rows; ++$i ) {
	# we only need y-coordinates
        (undef, my $pos) = $self->buffer_to_window_coords ('left', 0, $row_number_coords_y[$i]);

	# this only changes the color below the text, not for the whole left window...
	my $attrlist = Pango::AttrList->new;
	my $attribute = Pango::AttrBackground->new( 233*257, 189*257, 97*257);
	$attrlist->insert($attribute);
	$layout->set_attributes($attrlist);

	$layout->set_markup("$row_numbers[$i]");

        $self->style->paint_layout ($window, $self->state, FALSE, undef, $self, undef, $row_number_width + $row_number_margins,  $pos, $layout);

    }
}

sub redraw_property_view {
    my ($self, $event) = @_;
    
    # the following three lines only paint the left window black...
    # my $window = $self->get_window('left');   # the line numbers are in the left part of our textview
    # my $color=Gtk2::Gdk::Color->new( 233*257, 189*257, 97*257);
    # $window->set_background($color);
    $self->draw_line_numbers ($event);
    $self->signal_chain_from_overridden ($event);
    return FALSE;
}


################################
################################

package PropertyViewer;

use namespaces;
use strict;
use Gtk2 '-init';
use Glib qw/TRUE FALSE/; 
use Class::Struct;

sub tree_nodes {

  my ($tree_store, $object, $iter_parent) = @_;

  foreach my $pv (@{$object->contents}) {

    next if !defined($pv) || $pv->property->flags & Core::Property::Flags::is_non_storable;

    if (instanceof Core::Object($pv)) {
       my $iter = $tree_store->append($iter_parent);
       my $name=$pv->property->qualified_name;
       my $display_name = $name;
       $tree_store->set ($iter,0 => $name, 1 => $display_name);
       tree_nodes($tree_store, $pv, $iter);
    } elsif ($pv->property->flags & Core::Property::Flags::is_multiple) {
       my $count = 0;
       my @pv_array = @{$pv->values};
       my $name = $pv->property->qualified_name;
       my $length = scalar(@pv_array);
       foreach my $val (@pv_array) {
          my $iter = $tree_store->append($iter_parent);
          my $display_name = $name;
          if ($length!=1){
             $display_name.="($count)";
             $count++;
          }
          my $multiple_number = $count; # number in display_name+1 !
          $tree_store->set ($iter,0 => $name, 1 => $display_name, 2 => $multiple_number);
          tree_nodes($tree_store, $val, $iter);
       }
     } elsif (defined($pv->value) && !($pv->flags & PropertyValue::Flags::is_weak_ref)) {
       my $iter = $tree_store->append($iter_parent);
       my $name=$pv->property->qualified_name;
       my $display_name = $name;
       $tree_store->set ($iter,0 => $name, 1 => $display_name);
     }
  }

  #attachments
  while (my ($name, $at)=each %{$object->attachments}) {
     my $display_name = '*'.$name;
     my $is_attachment = 1;

     my $iter = $tree_store->append($iter_parent);
     $tree_store->set ($iter,0 => $name, 1 => $display_name, 3 => $is_attachment);
  }

}



sub build_object_tree {
   my ($object) = @_;

   my $tree_store = Gtk2::TreeStore->new(qw/Glib::String/, qw/Glib::String/, qw/Glib::Int/, qw/Glib::Boolean/);
   tree_nodes($tree_store, $object, undef);
   return $tree_store;

}

sub sub_obj_from_treestore {
    my ($cur_obj, $tree_store, $treeiter) = @_;
    my $name = $tree_store->get($treeiter,0);
    my $multiple_number = $tree_store->get($treeiter,2); #multiple number
    my $is_attachment = $tree_store->get($treeiter,3); #is attachment?
    if ( $multiple_number > 0) {
	return $cur_obj->give("$name")->[$multiple_number-1];
    } else {
	if ($is_attachment) {
	    return $cur_obj->get_attachment("$name");
	} else {
	    return $cur_obj->give("$name");
	}
    }
}


sub treeiter_array {
    my ($tree_store, $iter) = @_;
    my @iters = ($iter);
    while ($tree_store->iter_parent($iter)){
	my $parent = $tree_store->iter_parent($iter);
	push(@iters,$parent);
	$iter = $parent;
    }
    return \@iters;
}

sub last_obj_from_path {
    my ($object, $tree_store, $iter) = @_;
    my @iters = @{treeiter_array($tree_store, $iter)};
    my $cur_obj = $object;
    my $length = scalar(@iters);
    if ($length == 1) {
	$cur_obj = sub_obj_from_treestore($object, $tree_store, $iter);
    } else {
	for(my $i = $length-1; $i >= 0; --$i){ 
	    my $treeiter = $iters[$i];
	    $cur_obj = sub_obj_from_treestore($cur_obj, $tree_store, $treeiter);
	}	
    }
    return $cur_obj;
}


sub generate_tree_view { 
    my ($object, $tree_store, $text_view) = @_;
	my $tree_view = Gtk2::TreeView->new($tree_store);
    
        $tree_view->get_selection->signal_connect('changed' => 
	    sub {
		my ($selection) = @_;
		my $iter = $selection->get_selected;
		if ($iter) {
		    my $last_poly_obj = last_obj_from_path($object, $tree_store, $iter);
		    if ( $tree_store->iter_n_children($iter) != 0 ) {
			$text_view->get_buffer->set_text("polymake object ".$last_poly_obj->type->name);
		    } else { 
			$text_view->get_buffer->set_text($last_poly_obj);
		    }
		} else {
		    $text_view->get_buffer->set_text("no property selected");		    
		}
						  }
	    );
		
		#create a Gtk2::TreeViewColumn to add to $tree_view
		my $tree_column = Gtk2::TreeViewColumn->new();
		
		$tree_column->set_title ("Click to sort");
			
			#create a renderer that will be used to display info
			#in the model
			my $renderer = Gtk2::CellRendererText->new;
		#add this renderer to $tree_column. This works like a Gtk2::Hbox
		# so you can add more than one renderer to $tree_column			
		$tree_column->pack_start ($renderer, FALSE);
		
		 # set the cell "text" attribute to column 0   
		 #- retrieve text from that column in treestore 
		 # Thus, the "text" attribute's value will depend on the row's value
		 # of column 0 in the model($treestore),
		 # and this will be displayed by $renderer,
		 # which is a text renderer
		$tree_column->add_attribute($renderer, text => 1);
	
	#add $tree_column to the treeview
	$tree_view->append_column ($tree_column);
	
	 # make it searchable
   	$tree_view->set_search_column(1);
	
		# Allow sorting on the column
		$tree_column->set_sort_column_id(1);
   
	# Allow drag and drop reordering of rows
        $tree_view->set_reorderable(TRUE);

	return $tree_view;
}


sub property_view {
    my ($object) = @_; 

#standard window creation, placement, and signal connecting
my $window = Gtk2::Window->new('toplevel');
$window->set_title('polymake Property Viewer');
$window->signal_connect('delete_event' => sub { Gtk2->main_quit; });
$window->set_border_width(5);
$window->set_position('none');


#create data_structures
my $tree_store = build_object_tree($object);
my $text_view = PropertyTextView->new();
$text_view->set_wrap_mode('char');
my $content = "no property selected";
my $buffer = $text_view->get_buffer();
$buffer->set_text($content);
my $tree_view = generate_tree_view($object, $tree_store, $text_view);

# try to emit a signal that the window content has changed and the line numbers need to be redrawn
# does not work this way...
# $buffer->signal_connect ( changed => sub { $window->signal_emit("expose-event"); } ); 

#this vbox will geturn the bulk of the gui
my $collect = Gtk2::HBox->new(FALSE,5);
my $prop_display = &ret_prop_display($text_view);
my $prop_tree = &ret_prop_tree($object, $tree_view);
$collect->pack_start($prop_tree,TRUE,TRUE,0);
$collect->pack_start($prop_display,TRUE,TRUE,0);
$collect->show_all();

#add and show the vbox
$window->add($collect);
$window->show();

#our main event-loop
Gtk2->main();
}

sub ret_prop_display {
    my ($text_view) = @_;
my $prop_display = Gtk2::VBox->new(FALSE,5);
	my $sw = Gtk2::ScrolledWindow->new (undef, undef);
    		$sw->set_shadow_type ('etched-out');
		$sw->set_policy ('automatic', 'automatic');
		#This is a method of the Gtk2::Widget class,it will force a minimum 
		#size on the widget. Handy to give intitial size to a 
		#Gtk2::ScrolledWindow class object
		$sw->set_size_request (300, 300);
		#method of Gtk2::Container
		$sw->set_border_width(5);

                $sw->add($text_view);
	
$prop_display->pack_start($sw,TRUE,TRUE,0);
$prop_display->show_all();
return $prop_display;

}



sub ret_prop_tree {
    my ($object, $tree_view) = @_;
my $vbox = Gtk2::VBox->new(FALSE,5);

	#create a scrolled window that will host the treeview
	my $sw = Gtk2::ScrolledWindow->new (undef, undef);
    		$sw->set_shadow_type ('etched-out');
		$sw->set_policy ('automatic', 'automatic');
		#This is a method of the Gtk2::Widget class,it will force a minimum 
		#size on the widget. Handy to give intitial size to a 
		#Gtk2::ScrolledWindow class object
		$sw->set_size_request (300, 300);
		#method of Gtk2::Container
		$sw->set_border_width(5);
	
	$sw->add($tree_view);
	
$vbox->pack_start($sw,TRUE,TRUE,0);
$vbox->show_all();
return $vbox;
}



###################################################

sub propPathToString {
    my ($tree_store, $iter) = @_;
    my 	$property = $tree_store->get($iter,0);
    my $treeiter = $tree_store->iter_parent($iter);
    while ($treeiter != undef) {
	$property = $tree_store->get($treeiter,0) . "." . $property;
	$treeiter = $tree_store->iter_parent ($treeiter);	
    }
    return $property;
}


# this is the actual script calling the viewer...
property_view(@_);
