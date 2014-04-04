/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.toolsystem;

import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.toolsystem.config.ToolSystemConfiguration;
import de.jreality.toolsystem.config.VirtualMapping;
import de.jreality.util.LoggingSystem;

/**
 * 
 * TODO: document this
 * 
 * Essentially, this class maps tools to slots.
 * 
 * @author weissman
 *  
 */
public class SlotManager {

    /**
     * up-to-date map of (used) slots to activatable Tools
     */
    private final HashMap<InputSlot, HashSet<Tool>> slot2activation = new LinkedHashMap<InputSlot, HashSet<Tool>>();
    /**
     * up-to-date map of (used) slots to active Tools
     */
    private final HashMap<InputSlot, HashSet<Tool>> slot2active = new LinkedHashMap<InputSlot, HashSet<Tool>>();
    /**
     * up-to-date map of (used) slots to active Tools
     */
    private final HashMap<Tool, HashSet<InputSlot>> tool2currentSlots = new LinkedHashMap<Tool, HashSet<InputSlot>>();

    /**
     * up-to-date map of deactivation-slots to active Tools
     */
    private final HashMap<InputSlot, HashSet<Tool>> slot2deactivation = new LinkedHashMap<InputSlot, HashSet<Tool>>();
    
    private final HashMap<InputSlot, HashSet<InputSlot>> virtualMappings = new LinkedHashMap<InputSlot, HashSet<InputSlot>>();
    private final HashMap<InputSlot, HashSet<InputSlot>> virtualMappingsInv = new LinkedHashMap<InputSlot, HashSet<InputSlot>>();
    
    private final HashMap<Tool, HashMap<InputSlot, InputSlot>> slotsToMappingsForTool = new LinkedHashMap<Tool, HashMap<InputSlot, InputSlot>>();
    
    private final HashMap<Tool, HashSet<InputSlot>> virtualSlotsForTool = new LinkedHashMap<Tool, HashSet<InputSlot>>();

    SlotManager(ToolSystemConfiguration config) {
      List virtualMappings = config.getVirtualMappings();
      for (Iterator i = virtualMappings.iterator(); i.hasNext(); ) {
        VirtualMapping vm = (VirtualMapping) i.next();
        getMappingsSourceToTargets(vm.getSourceSlot()).add(vm.getTargetSlot());
        getMappingsTargetToSources(vm.getTargetSlot()).add(vm.getSourceSlot());
      }
    }
    
    private Set<InputSlot> getMappingsSourceToTargets(InputSlot slot) {
      if (!virtualMappings.containsKey(slot))
        virtualMappings.put(slot, new HashSet<InputSlot>());
      return virtualMappings.get(slot);
    }
    
    private Set<InputSlot> getMappingsTargetToSources(InputSlot slot) {
      if (!virtualMappingsInv.containsKey(slot))
        virtualMappingsInv.put(slot, new HashSet<InputSlot>());
      return virtualMappingsInv.get(slot);
    }

    /**
     * returns a map that maps "raw" slots to slotnames for each tool
     * - this map is up to date:
     * * contains activation slots if tool is inactive
     * * contains activation slots + current slots if tool is activated
     * * contains current slots if tool is always active
     * @param tool 
     * @return the map described above
     */
    private Map<InputSlot, InputSlot> getMappingsForTool(Tool tool) {
      if (!slotsToMappingsForTool.containsKey(tool))
        slotsToMappingsForTool.put(tool, new HashMap<InputSlot, InputSlot>());
      return slotsToMappingsForTool.get(tool);
    }

    Set<Tool> getToolsActivatedBySlot(InputSlot slot) {
        return Collections.unmodifiableSet(getSlot2activation(slot));
    }

    private Set<Tool> getSlot2activation(InputSlot slot) {
        if (!slot2activation.containsKey(slot))
                slot2activation.put(slot, new HashSet<Tool>());
        return slot2activation.get(slot);
    }

    Set<Tool> getToolsDeactivatedBySlot(InputSlot slot) {
        return Collections.unmodifiableSet(getSlot2deactivation(slot));
    }

    private Set<Tool> getSlot2deactivation(InputSlot slot) {
        if (!slot2deactivation.containsKey(slot))
                slot2deactivation.put(slot, new HashSet<Tool>());
        return slot2deactivation.get(slot);
    }

    Set<Tool> getActiveToolsForSlot(InputSlot slot) {
        return Collections.unmodifiableSet(getSlot2active(slot));
    }

    private Set<Tool> getSlot2active(InputSlot slot) {
        if (!slot2active.containsKey(slot))
                slot2active.put(slot, new HashSet<Tool>());
        return slot2active.get(slot);
    }

    private Set<InputSlot> getTool2currentSlots(Tool tool) {
        if (!tool2currentSlots.containsKey(tool))
            tool2currentSlots.put(tool, new HashSet<InputSlot>());
        return tool2currentSlots.get(tool);
    }

    boolean isActiveSlot(InputSlot slot) {
        return slot2active.containsKey(slot);
    }

    boolean isActivationSlot(InputSlot slot) {
        return slot2activation.containsKey(slot);
    }

    private Set<InputSlot> getVirtualSlotsForTool(Tool tool) {
      if (!virtualSlotsForTool.containsKey(tool))
        virtualSlotsForTool.put(tool, new HashSet<InputSlot>());
      return virtualSlotsForTool.get(tool);
  }

    /**
     * returns the original (trigger) slots for the given slot
     * @param slot
     * @return
     */
    Set<InputSlot> resolveSlot(InputSlot slot) {
      HashSet<InputSlot> ret = new LinkedHashSet<InputSlot>();
      findTriggerSlots(ret, slot);
      return ret;
    }
    private void findTriggerSlots(Set<InputSlot> l, InputSlot slot) {
      Set<InputSlot> sources = getMappingsTargetToSources(slot);
      //Set sources = getMappingsSourceToTargets(slot);
      if (sources.isEmpty()) {
        l.add(slot);
        return;
      }
      for (InputSlot sl : sources)
        findTriggerSlots(l, sl);
    }
    private Set<InputSlot> resolveSlots(List<InputSlot> slotSet) {
      Set<InputSlot> ret = new LinkedHashSet<InputSlot>();
      for (InputSlot slot : slotSet )
        findTriggerSlots(ret, slot);
      return ret;
    }

    /**
     * updates the maps for the current tool system state
     * 
     * @param activeTools tools that are still active
     * @param activatedTools tools activated recently
     * @param deactivatedTools tools deactivated recently
     */
    void updateMaps(final Set<Tool> activeTools, final Set<Tool> activatedTools, final Set<Tool> deactivatedTools) {
        // handle newly activated tools
        for (Tool tool : activatedTools) {
            // update slot2active
            for (InputSlot slot : tool.getCurrentSlots()) {
                for (InputSlot resolvedSlot : resolveSlot(slot)) {
                  getSlot2active(resolvedSlot).add(tool);
                  getMappingsForTool(tool).put(resolvedSlot, slot);
                }
            }
            
            // remember all currently used slots for activated tool
            Set<InputSlot> currentSlots = resolveSlots(tool.getCurrentSlots());
            getTool2currentSlots(tool).addAll(currentSlots);
            getVirtualSlotsForTool(tool).addAll(tool.getCurrentSlots());

            // update slot2activation
            for (InputSlot slot : tool.getActivationSlots()) {
	            for (InputSlot resolvedSlot : resolveSlot(slot)) {
	              getSlot2activation(resolvedSlot).remove(tool);
	              getSlot2deactivation(resolvedSlot).add(tool);
	            }
            }
        }

        // handle newly deactivated tools
        for (Tool tool : deactivatedTools) {
            // update slot2active
            for (InputSlot slot : tool.getCurrentSlots()) {
                for (InputSlot resolvedSlot : resolveSlot(slot)) {
                getSlot2active(resolvedSlot).remove(tool);
                getMappingsForTool(tool).remove(resolvedSlot);
              }
            }
            // update slot2activation
            for (InputSlot slot : tool.getActivationSlots()) {
	            for (InputSlot resolvedSlot : resolveSlot(slot)) {
	              getSlot2activation(resolvedSlot).add(tool);
	              getSlot2deactivation(resolvedSlot).remove(tool);
	            }
            }            
            getVirtualSlotsForTool(tool).clear();
            getVirtualSlotsForTool(tool).addAll(tool.getActivationSlots());
        }
        
        //update used slots for still active tools
        for (Tool tool : activeTools) {
            Set<InputSlot> newUsed = resolveSlots(tool.getCurrentSlots());
            Set<InputSlot> oldUsed = getTool2currentSlots(tool);
            // contains all newly used slots for the tool
            Set<InputSlot> added = new HashSet<InputSlot>(newUsed);
            added.removeAll(oldUsed);
            // contains all no-longer-used slots for the tool
            Set<InputSlot> removed = new HashSet<InputSlot>(oldUsed);
            removed.removeAll(newUsed);
            for (InputSlot slot : added) 
                getSlot2active(slot).add(tool);

            for (InputSlot slot : removed)
                getSlot2active(slot).remove(tool);
            
            getTool2currentSlots(tool).removeAll(removed);
            getTool2currentSlots(tool).addAll(added);
            
            Set<InputSlot> oldUsedVirtual = new HashSet<InputSlot>(getVirtualSlotsForTool(tool));

            Set<InputSlot> newUsedVirtual = new HashSet<InputSlot>();
            newUsedVirtual.addAll(tool.getActivationSlots());
            newUsedVirtual.addAll(tool.getCurrentSlots());
            
            oldUsedVirtual.removeAll(newUsedVirtual); // these are no longer used
            
            newUsedVirtual.removeAll(getVirtualSlotsForTool(tool)); // these are new used
            
            // update the map
            getVirtualSlotsForTool(tool).removeAll(oldUsedVirtual);
            getVirtualSlotsForTool(tool).addAll(newUsedVirtual);

            getMappingsForTool(tool).keySet().removeAll(oldUsedVirtual);
            
//            for (Iterator j = getMappingsForTool(tool).values().iterator(); j.hasNext(); ) {
//              if (oldUsedVirtual.contains(j.next())) j.remove();
//            }
//            for (Iterator j = removed.iterator(); j.hasNext(); ) {
//              InputSlot slot = (InputSlot) j.next();
//              Set origSlots = resolveSlot(slot);
//              for (Iterator k = origSlots.iterator(); k.hasNext(); )
//                getMappingsForTool(tool).remove(k.next());
//            }

            for (InputSlot newSlot : newUsedVirtual) {
              for (InputSlot virtualSlot : resolveSlot(newSlot) )
                getMappingsForTool(tool).put(virtualSlot, newSlot);
            }
            
        }
    }

    /**
     * @param tool
     */
  void registerTool(Tool tool) {
    if (tool.getActivationSlots().isEmpty()) {
      // permanently active tool
      getVirtualSlotsForTool(tool).addAll(tool.getCurrentSlots());
      for (Iterator i = tool.getCurrentSlots().iterator(); i.hasNext();) {
        InputSlot mappedSlot = (InputSlot) i.next();
        getTool2currentSlots(tool).addAll(resolveSlot(mappedSlot));
        for (Iterator i2 = resolveSlot(mappedSlot).iterator(); i2.hasNext();) {
          InputSlot resolvedSlot = (InputSlot) i2.next();
          getMappingsForTool(tool).put(resolvedSlot, mappedSlot);
          getSlot2active(resolvedSlot).add(tool);
        }
      }
    } else {
      getVirtualSlotsForTool(tool).addAll(tool.getActivationSlots());
      
      //InputSlot mappedSlot = tool.getActivationSlot();
      for (InputSlot mappedSlot : tool.getActivationSlots()) {
	      Set resolvedSlots = resolveSlot(mappedSlot);
	      for (Iterator i2 = resolvedSlots.iterator(); i2.hasNext();) {
	        InputSlot resolvedSlot = (InputSlot) i2.next();
	        getMappingsForTool(tool).put(resolvedSlot, mappedSlot);
	        getSlot2activation(resolvedSlot).add(tool);
	      }
      }
    }
    LoggingSystem.getLogger(this).info(
        "registered Tool " + tool.getClass().getName());
  }

  void unregisterTool(Tool tool) {
    if (tool.getActivationSlots().isEmpty()) {
      // permanently active tool
      for (Iterator i = tool.getCurrentSlots().iterator(); i.hasNext();) {
        InputSlot mappedSlot = (InputSlot) i.next();
        Set resolvedSlots = resolveSlot(mappedSlot);
        for (Iterator i2 = resolvedSlots.iterator(); i2.hasNext();) {
          InputSlot resolvedSlot = (InputSlot) i2.next();
          getSlot2active(resolvedSlot).remove(tool);
        }
      }
    } else {
      // InputSlot mappedSlot = tool.getActivationSlot();
      for (InputSlot mappedSlot : tool.getActivationSlots()) {
	      Set resolvedSlots = resolveSlot(mappedSlot);
	      for (Iterator i2 = resolvedSlots.iterator(); i2.hasNext();) {
	        InputSlot resolvedSlot = (InputSlot) i2.next();
	        getSlot2activation(resolvedSlot).remove(tool);
	      }
      }
    }
    getMappingsForTool(tool).clear();
    getTool2currentSlots(tool).clear();
    getVirtualSlotsForTool(tool).clear();
    LoggingSystem.getLogger(this).info(
        "unregistered Tool " + tool.getClass().getName());
  }

    /**
     * returns the name under which the given slot is expected by the tool
     * 
     * @param tool
     * @param sourceSlot
     * @return the sourceSlot or the mapped slot (which is handled by the tool)
     */
    InputSlot resolveSlotForTool(Tool tool, InputSlot sourceSlot) {
    	InputSlot ret = (InputSlot) getMappingsForTool(tool).get(sourceSlot);
    	return ret == null ? sourceSlot : ret;
    }
}
