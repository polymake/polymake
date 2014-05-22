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

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;
import java.util.WeakHashMap;

import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.pick.AABBPickSystem;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.pick.PickSystem;
import de.jreality.scene.pick.PosWHitFilter;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.tools.AnimatorTool;
import de.jreality.toolsystem.config.ToolSystemConfiguration;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;
import de.jreality.util.RenderTrigger;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

/**
 * 
 * TODO: document this
 * 
 * @author weissman
 *  
 */
public class ToolSystem implements ToolEventReceiver {

    static WeakHashMap<Viewer, ToolSystem> globalTable = new WeakHashMap<Viewer, ToolSystem>();
    
	/**
	 * If <i>v</i> has a tool system already associated to it, return it. Otherwise allocate a default one
	 * @param v
	 * @return
	 */
    public static ToolSystem toolSystemForViewer(Viewer v)	{
		synchronized (globalTable) {
			ToolSystem sm = (ToolSystem) globalTable.get(v);
			if (sm != null) return sm;
			LoggingSystem.getLogger(ToolSystem.class).warning("Viewer has no tool system, allocating default");
			sm = new ToolSystem(v, null, null);
			globalTable.put(v,sm);
			return sm;
		}
	}
	
	/**
	 * This method just looks up and returns the possibly null toolsystem associated to viewer
	 * @param v
	 * @return
	 */
	 public static ToolSystem getToolSystemForViewer(Viewer v)	{
		 synchronized (globalTable) {
			 ToolSystem sm = (ToolSystem) globalTable.get(v);
			 return sm;
		 }
	}
	
	public static void setToolSystemForViewer(Viewer v, ToolSystem ts)	{
		synchronized (globalTable) {
			ToolSystem sm = (ToolSystem) globalTable.get(v);
			if (sm != null) throw new IllegalStateException("Viewer already has tool system "+sm);
			globalTable.put(v,ts);
		}
	}
	
	private static void unsetToolSystem(ToolSystem ts) {
		synchronized (globalTable) {
			for (Iterator<Entry<Viewer, ToolSystem>> it = globalTable.entrySet().iterator(); it.hasNext(); ) {
				Entry<Viewer, ToolSystem> e = it.next();
				if (e.getValue() == ts) it.remove();
			}
		}
	}
	
	private RenderTrigger renderTrigger;

	protected final LinkedList<ToolEvent> compQueue = new LinkedList<ToolEvent>();

	private final LinkedList<ToolEvent> triggerQueue = new LinkedList<ToolEvent>();
	private final HashMap<Tool, List<SceneGraphPath>> toolToPath = new HashMap<Tool, List<SceneGraphPath>>();
	private List<PickResult> pickResults = Collections.emptyList();
	private PosWHitFilter hitFilter;

	private SceneGraphPath emptyPickPath=new SceneGraphPath();

	protected Viewer viewer;
	private ToolContextImpl toolContext;
	protected DeviceManager deviceManager;
	private ToolManager toolManager;
	private SlotManager slotManager;
	private PickSystem pickSystem;
	private ToolUpdateProxy updater;
	private ToolEventQueue eventQueue;
	ToolSystemConfiguration config;
	private PickResult pickResult;
	private static InputSlot pointerSlot = InputSlot.getDevice("PointerTransformation");

	protected boolean executing;

	private final Object KEY=new Object();
	
	private class ToolContextImpl implements ToolContext {

		InputSlot sourceSlot;
		ToolEvent event;
		private SceneGraphPath rootToLocal;
		private SceneGraphPath rootToToolComponent;
		private Tool currentTool;

		boolean rejected;

		public Viewer getViewer() {
			return viewer;
		}

		public InputSlot getSource() {
			return event.getInputSlot();
		}

		public DoubleArray getTransformationMatrix(InputSlot slot) {
			return deviceManager.getTransformationMatrix(slot);
		}

		public AxisState getAxisState(InputSlot slot) {
			return deviceManager.getAxisState(slot);
		}

		public long getTime() {
			return event.getTimeStamp();
		}

		private void setRootToLocal(SceneGraphPath rootToLocal) {
			this.rootToLocal = rootToLocal;
			rootToToolComponent = null;
		}

		public SceneGraphPath getRootToLocal() {
			return rootToLocal;
		}

		public SceneGraphPath getRootToToolComponent() {
			if (rootToToolComponent == null) {
				LinkedList<SceneGraphNode> list = new LinkedList<SceneGraphNode>();
				for (Iterator<SceneGraphNode> i = rootToLocal.reverseIterator(); i.hasNext(); ) {
					SceneGraphNode cp = i.next();
					if (!(cp instanceof SceneGraphComponent))
						continue;
					if (((SceneGraphComponent) cp).getTools().contains(currentTool)) {
						list.addFirst(cp);
						while (i.hasNext())
							list.addFirst((SceneGraphNode) i.next());
					}
				}
				rootToToolComponent = SceneGraphPath.fromList(list);
			}
			return rootToToolComponent;
		}

		public PickResult getCurrentPick() {
			if (pickResult == null) {
				performPick();
				pickResult = pickResults.isEmpty() ? null : (PickResult) pickResults.get(0);				
			}
			return pickResult;
		}

		public List<PickResult> getCurrentPicks() {
			if (pickResults == null) {
				performPick();
			}
			return pickResults;
		}

		private void setCurrentTool(Tool currentTool) {
			this.currentTool = currentTool;
		}

		public void reject() {
			rejected=true;
		}

		boolean isRejected() {
			return rejected;
		}

		public SceneGraphPath getAvatarPath() {
			return ToolSystem.this.getAvatarPath();
		}

		public PickSystem getPickSystem() {
			return ToolSystem.this.getPickSystem();
		}

		public Object getKey() {
			return KEY;
		}
	};

	private static ToolSystemConfiguration loadConfiguration() {
	    ToolSystemConfiguration config;
	    try {
	      String toolFile = Secure.getProperty(SystemProperties.TOOL_CONFIG_FILE);
	      config = ToolSystemConfiguration.loadConfiguration(
	          Input.getInput(toolFile)
	      );
	      LoggingSystem.getLogger(ToolSystem.class).config("Using toolconfig="+toolFile);
	    } catch (Exception e1) {
	      config = ToolSystemConfiguration.loadDefaultConfiguration();
	    }
	    return config;
	  }
	/**
	 * 
	 * @param viewer the viewer
	 * @param config the config
	 * @param renderTrigger a rendertrigger to synch or null - the ToolSystem does not take care of
	 * setting/removing the triggers viewer and scene root (on initialize/dispose)
	 */
	public ToolSystem(Viewer viewer, ToolSystemConfiguration config, RenderTrigger renderTrigger) {
		toolContext = new ToolContextImpl();
		toolManager = new ToolManager();
		eventQueue = new ToolEventQueue(this);
		if (config == null) config = loadConfiguration();
		this.config = config;
		this.viewer = viewer;
		deviceManager = new DeviceManager(config, eventQueue, viewer);
		slotManager = new SlotManager(config);
		updater = new ToolUpdateProxy(this);
		this.renderTrigger = renderTrigger;
		// this code moved over from the ToolSystemViewer constructor
	    setPickSystem(new AABBPickSystem());
	    // provide a reasonable default empty pick path
	    emptyPickPath = new SceneGraphPath();
	    emptyPickPath.push(viewer.getSceneRoot());
	}

	private class MouseOverSupport implements Tool {

		List<InputSlot> activation = Collections.emptyList(); //Collections.singletonList(InputSlot.getDevice("EnablePointerHit"));
		List<InputSlot> pointer = Collections.singletonList(InputSlot.getDevice("PointerTransformation"));
		InputSlot trigger = InputSlot.getDevice("PointerHit");
		
		
		SceneGraphPath rootPath;
		int useCount=0;
		
		void mouseOverToolAdded() {
			if (useCount == 0) {
				//System.out.println("Enabling mouse over support");
				addToolImpl(this, rootPath);
			}
			useCount++;
		}
		
		void mouseOverToolRemoved() {
			useCount--;
			if (useCount == 0) {
				//System.out.println("Disabling mouse over support");
				removeToolImpl(this, rootPath);
			}
		}
		
		private MouseOverSupport(SceneGraphPath root) {
			rootPath=new SceneGraphPath(root);
		}
		
		public void activate(ToolContext tc) {
		}

		public void deactivate(ToolContext tc) {
		}

		public List<InputSlot> getActivationSlots() {
			return activation;
		}

		public List<InputSlot> getCurrentSlots() {
			return pointer;
		}

		public String getDescription(InputSlot slot) {
			return "foo";
		}

		public String getDescription() {
			return "dummy tool to enable mouse over";
		}
		
		boolean hasHit=false;
		SceneGraphPath lastPath=null;
		int ignoreCnt=12;
		public void perform(ToolContext tc) {
			if (ignoreCnt > 0) {
				ignoreCnt--;
				return;
			}
			SceneGraphPath newP = null;
			boolean hits = false;
			PickResult p = tc.getCurrentPick();
			if (p!=null && p.getPickPath() != null && p.getPickPath().getLastElement() instanceof Geometry) {
				newP = new SceneGraphPath(p.getPickPath());
				hits = true;
			}
			if (!hits) {
				if (!hasHit) {
					// nothing to do...
				} else {
					hasHit = false;
					lastPath = null;
					fireNoMoreHit();
				}
			} else {
				if (hasHit) {
					if (lastPath.isEqual(newP)) {
						// same hit, nothing to do
					}
					else {
						lastPath = newP;
						// fire hit lost
						fireNoMoreHit();
						// then fire hit again
						fireHit();
					}
				} else {
					lastPath = newP;
					hasHit = true;
					fireHit();
				}
					
			}
		}

		private void fireNoMoreHit() {
			eventQueue.addEvent(new ToolEvent(this, deviceManager.getSystemTime(), trigger, AxisState.ORIGIN));
		}

		private void fireHit() {
			eventQueue.addEvent(new ToolEvent(this, deviceManager.getSystemTime(), trigger, AxisState.PRESSED));
		}

		@Override
		public int hashCode() {
			return 31;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			throw new IllegalStateException("Duplicate MouseOverSupport!");
		}
		
	}
	
	private boolean initialized;

	private MouseOverSupport mouseOverSupport;
	public void initializeSceneTools() {
		if (initialized) {
			LoggingSystem.getLogger(this).warning("already initialized!");
			return;
		}
		initialized=true;
		toolManager.cleanUp();
		
		// register animator
		SceneGraphPath rootPath = new SceneGraphPath();
		rootPath.push(viewer.getSceneRoot());
		addTool(AnimatorTool.getInstanceImpl(KEY), rootPath);
		
		// enable mouse over support
		mouseOverSupport = new MouseOverSupport(rootPath);
		
		if (emptyPickPath.getLength() == 0) {
			emptyPickPath.push(viewer.getSceneRoot());
		}
		if (pickSystem != null) {
			pickSystem.setSceneRoot(viewer.getSceneRoot());
		}
		updater.setSceneRoot(viewer.getSceneRoot());
		eventQueue.start();
	}

	public void processToolEvent(ToolEvent event) {
		synchronized (mutex) {
			if (disposed) return;
			executing=true;
		}
		compQueue.add(event);
		int iterCnt=0;
		do {
			iterCnt++;
			processComputationalQueue();
			processTriggerQueue();
			List<ToolEvent> l = deviceManager.updateImplicitDevices();
			if (l.isEmpty()) break;
			compQueue.addAll(l);
			if (iterCnt > 5000) {
				//throw new IllegalStateException("recursion in tool system!");
				LoggingSystem.getLogger(this).warning("may be stuck in endless loop");
				iterCnt = 0;
			}
		} while (true);
		// handle newly added/removed tools
		synchronized (mutex) {
			if (!toolsChanging.isEmpty()) {
				final List<Pair> l = new LinkedList<Pair>(toolsChanging);
				toolsChanging.clear();
				for (Iterator<Pair> i = l.iterator(); i.hasNext(); ) {
					Pair p = i.next();
					i.remove();
					if (p.added) {
						addToolImpl(p.tool, p.path);
					} else {
						removeToolImpl(p.tool, p.path);
					}
				}
			}
			executing=false;
		}
		if (event.getInputSlot() == InputSlot.getDevice("SystemTime")) {
			deviceManager.setSystemTime(event.getTimeStamp());
			if (renderTrigger != null) {
				renderTrigger.finishCollect();
				renderTrigger.startCollect();
			}
		}
	}

	protected void processComputationalQueue() {
		while (!compQueue.isEmpty()) {
			ToolEvent event = (ToolEvent) compQueue.removeFirst();
			deviceManager.evaluateEvent(event, compQueue);
			if (isTrigger(event) && !event.isConsumed()) triggerQueue.add(event);
		}
	}

	private boolean isTrigger(ToolEvent event) {
		InputSlot slot = event.getInputSlot();
		boolean ret = slotManager.isActiveSlot(slot) || slotManager.isActivationSlot(slot);
		return ret;
	}

	protected void processTriggerQueue() {
		if (triggerQueue.isEmpty())	return;

		HashSet<Tool> activatedTools = new HashSet<Tool>();
		HashSet<Tool> deactivatedTools = new HashSet<Tool>();
		HashSet<Tool> stillActiveTools = new HashSet<Tool>();

		SceneGraphPath pickPath = null;

		for (ToolEvent event : triggerQueue) { //Iterator iter = triggerQueue.iterator(); iter.hasNext();) {
//			ToolEvent event = (ToolEvent) iter.next();
			toolContext.event = event;
			InputSlot slot = event.getInputSlot();
			toolContext.sourceSlot = slot;
			pickResults = null;
			pickResult = null;
			
			AxisState axis = deviceManager.getAxisState(slot);

			boolean noTrigger = true;

			if (axis != null && axis.isPressed()) { // possible activation:

				Set<Tool> candidatesForPick = new HashSet<Tool>(slotManager.getToolsActivatedBySlot(slot));

				Set<Tool> candidates = new HashSet<Tool>();

				// TODO: see if activating more than one Tool for an axis
				// makes sense...
				for (Tool candidate : candidatesForPick) {
					if (!toolManager.needsPick(candidate)) //throw new Error();
						LoggingSystem.getLogger(this).warning("Something wrong with pick candidates\n");
				}
				if (!candidatesForPick.isEmpty()) {
					// now we need a pick path
					if (pickPath == null)
						pickPath = calculatePickPath();
					int level = pickPath.getLength();
					do {
						Collection<Tool> selection = toolManager.selectToolsForPath(pickPath, level--, candidatesForPick);
						if (selection.isEmpty()) continue;
						LoggingSystem.getLogger(this).finer("selected pick tools:" + selection);
						for (Tool tool : selection)   {
							registerActivePathForTool(pickPath, tool);
						}
						candidates.addAll(selection);
						// now all Tools in the candidates list need to be
						// processed=activated
						activateToolSet(candidates);
					} while (candidates.isEmpty() && level > 0);
					activatedTools.addAll(candidates);
					noTrigger = candidates.isEmpty();
				}
			}
			if (axis != null && axis.isReleased()) { // possible deactivation
				Set<Tool> deactivated = findDeactivatedTools(slot);
				deactivatedTools.addAll(deactivated);
				deactivateToolSet(deactivated);
				noTrigger = deactivated.isEmpty();
			}

			// process all active tools NEW: only if no tool was (de)activated
			if (noTrigger) {  //activatedTools.isEmpty() && deactivatedTools.isEmpty()
				Set<Tool> active = slotManager.getActiveToolsForSlot(slot);
				stillActiveTools.addAll(active);
				processToolSet(active);
			}
		}
		triggerQueue.clear();
		// NEW: this is now obsolete
		// // don't update used slots for deactivated tools!
		// stillActiveTools.removeAll(deactivatedTools);
		slotManager.updateMaps(stillActiveTools, activatedTools, deactivatedTools);
	}

	private void registerActivePathForTool(SceneGraphPath pickPath, Tool tool) {
		List<SceneGraphPath> ap = Collections.singletonList(
				pickPath.getLastElement() instanceof Geometry ? pickPath.popNew() : pickPath
						);
		toolToPath.put(tool, ap);
	}

	private double[] pointerTrafo = new double[16];

	private double[] currentPointer = new double[16];

	protected final Object mutex=new Object();

	private SceneGraphPath avatarPath;

	private boolean disposed;

	private void performPick() {
		if (pickSystem == null) {
			pickResults = Collections.emptyList();
			return;
		}
		pointerSlot = InputSlot.getDevice("PointerTransformation");
		currentPointer = deviceManager.getTransformationMatrix(pointerSlot).toDoubleArray(currentPointer);
		//if (Rn.equals(pointerTrafo, currentPointer)) return;
		Rn.copy(pointerTrafo, currentPointer);
		double[] to = new double[] { -pointerTrafo[2], -pointerTrafo[6],
				-pointerTrafo[10], -pointerTrafo[14] };
		double[] from = new double[] { pointerTrafo[3], pointerTrafo[7],
				pointerTrafo[11], pointerTrafo[15] };
		try {
			AABBPickSystem.getFrustumInterval(from, to, viewer);
		} catch (IllegalStateException e) {
			// no camera?
		}
		pickResults =  pickSystem.computePick(from, to);
		if (!SystemProperties.isPortal) {
			if (hitFilter == null)
				hitFilter = new PosWHitFilter(viewer);
			hitFilter.update();
			// throw out pick results whose NDC w-coordinate is negative
			AABBPickSystem.filterList(hitFilter, from, to, pickResults);			
		}
//		if (pickResults.size() != 0) {
//			System.err.println(SystemProperties.hostname+" Got "+pickResults.size()+" picks");
//			System.err.println(SystemProperties.hostname+" picked "+pickResults.get(0).getPickPath().getLastComponent().getName());
//		}
	}

	private SceneGraphPath calculatePickPath() {
		performPick();
		if (pickResults.isEmpty()) {
			return emptyPickPath;
		}
		PickResult result = (PickResult) pickResults.get(0);
		LoggingSystem.getLogger(this).fine("ToolSystem.calculatePickPath() <HIT>");
		return result.getPickPath();
	}

	/**
	 * calls perform(ToolContext tc) for all tools in the given Set
	 * removes Tools from the set if the tool rejected the activation.
	 * 
	 * @param toolSet
	 * @return false if the current level of tools rejected the context...
	 */
	private void activateToolSet(Set<Tool> toolSet) {
		for (Iterator<Tool> iter = toolSet.iterator(); iter.hasNext();) {
			Tool tool = iter.next();
			toolContext.setCurrentTool(tool);
			toolContext.event.device=slotManager.resolveSlotForTool(tool, toolContext.sourceSlot);
			if (toolContext.event.device == null) {
				LoggingSystem.getLogger(this).warning("activate: resolving "+toolContext.sourceSlot+" failed: "+tool.getClass().getName());
			}
			for (SceneGraphPath path : getActivePathsForTool(tool)) {
				toolContext.setRootToLocal(path);
				tool.activate(toolContext);
				if (toolContext.isRejected()) {
					iter.remove();
					toolContext.rejected=false;
				}
			}
		}
	}

	/**
	 * calls perform(ToolContext tc) for all tools in the given Set
	 * 
	 * @param toolSet
	 */
	private void processToolSet(Set<Tool> toolSet) {
		for (Tool tool : toolSet) {
			toolContext.setCurrentTool(tool);
			toolContext.event.device=slotManager.resolveSlotForTool(tool, toolContext.sourceSlot);
			for (SceneGraphPath path : getActivePathsForTool(tool)) {
				toolContext.setRootToLocal(path);
				tool.perform(toolContext);
			}
		}
	}

	private List<SceneGraphPath> getActivePathsForTool(Tool tool) {
		List<SceneGraphPath> l = toolToPath.get(tool);
		if (l == null) return Collections.emptyList();
		return l;
	}

	/**
	 * calls perform(ToolContext tc) for all tools in the given Set
	 * 
	 * @param toolSet
	 */
	private void deactivateToolSet(Set<Tool> toolSet) {
		for (Tool tool : toolSet) {
			toolContext.setCurrentTool(tool);
			toolContext.event.device=slotManager.resolveSlotForTool(tool, toolContext.sourceSlot);
			if (toolContext.event.device == null) {
				LoggingSystem.getLogger(this).warning("deavtivate: resolving "+toolContext.sourceSlot+" failed: "+tool.getClass().getName());
			}
			for (SceneGraphPath path : getActivePathsForTool(tool)) {
				toolContext.setRootToLocal(path);
				tool.deactivate(toolContext);
			}
		}
	}

	/**
	 * the given slot must have AxisState.isReleased() == true
	 * this is garanteed in processTrigger...
	 * 
	 * @param slot
	 * @return
	 */
	private Set<Tool> findDeactivatedTools(InputSlot slot) {
		return slotManager.getToolsDeactivatedBySlot(slot);
	}

	public void setPickSystem(PickSystem pickSystem) {
		this.pickSystem = pickSystem;
		if (pickSystem != null) {
			pickSystem.setSceneRoot(viewer.getSceneRoot());
		}
	}

	public PickSystem getPickSystem() {
		return pickSystem;
	}

	public void setAvatarPath(SceneGraphPath p) {
		avatarPath=p;
		deviceManager.setAvatarPath(avatarPath);
	}

	public SceneGraphPath getAvatarPath() {
		return avatarPath != null ? avatarPath : viewer.getCameraPath();
	}

	public void dispose() {
		synchronized (mutex) {
			disposed=true;
			while (executing)
				try {
					mutex.wait(1);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
		}
		System.out.println("event queue shut down done...");
		deviceManager.dispose();
		eventQueue.dispose();
		updater.dispose();
		AnimatorTool.disposeInstance(KEY);
		unsetToolSystem(this); // remove from the viewer->tool-system table
	}

	final List<Pair> toolsChanging = new LinkedList<Pair>();

	protected static class Pair {
		final Tool tool;
		final SceneGraphPath path;
		final boolean added;
		Pair(Tool tool, SceneGraphPath path, boolean added) {
			this.path=path;
			this.tool=tool;
			this.added=added;
		}
	}

	void addTool(Tool tool, SceneGraphPath path) {
		synchronized (mutex) {
			if (disposed) return;
			if (executing)
				toolsChanging.add(new Pair(tool, path, true));
			else addToolImpl(tool, path);
		}
	}

	void removeTool(Tool tool, SceneGraphPath path) {
		synchronized (mutex) {
			if (disposed) return;
			if (executing)
				toolsChanging.add(new Pair(tool, path, false));
			else removeToolImpl(tool, path);
		}
	}

	/**
	 * TODO: sync
	 * @param tool
	 * @param path
	 */
	void addToolImpl(Tool tool, SceneGraphPath path) {
		boolean first = toolManager.addTool(tool, path);
		if (!toolManager.needsPick(tool)) {
			List<SceneGraphPath> l = toolToPath.get(tool);
			if (l == null) {
				l = new LinkedList<SceneGraphPath>();
				toolToPath.put(tool, l);
			}
			try {
				l.add(path);
			} catch (UnsupportedOperationException e) {
				System.out.println("try adding to sigleton: "+tool);
			}
		}
		if (first) {
			slotManager.registerTool(tool);
			if (tool.getActivationSlots().contains(InputSlot.POINTER_HIT)) {
				mouseOverSupport.mouseOverToolAdded();
			}
		}
//		LoggingSystem.getLogger(this).finer(
//				"first=" + first + " tool=" + tool + "   path=" + path);
	}

	/**
	 * TODO: sync
	 * @param tool
	 * @param path
	 */
	void removeToolImpl(Tool tool, SceneGraphPath path) {
		boolean last = toolManager.removeTool(tool, path);
		for (SceneGraphPath activePath : getActivePathsForTool(tool)) {
			if (path.isEqual(activePath)) {
				ToolEvent te = new ToolEvent(this, -1, InputSlot.getDevice("remove"), null,	null);
				toolContext.setCurrentTool(tool);
				toolContext.setRootToLocal(path);
				toolContext.event = te;
				tool.deactivate(toolContext);
				toolToPath.remove(tool);
			}
		}
		if (last) {
			slotManager.unregisterTool(tool);
			if (tool.getActivationSlots().contains(InputSlot.POINTER_HIT)) {
				mouseOverSupport.mouseOverToolRemoved();
			}
		}
//		LoggingSystem.getLogger(this).finer(
//				"last=" + last + " tool=" + tool + " path=" + path);
	}

	public SceneGraphPath getEmptyPickPath() {
		return emptyPickPath;
	}

	public void setEmptyPickPath(SceneGraphPath emptyPickPath) {
		if (emptyPickPath != null) {
			if (emptyPickPath.getFirstElement().getName() != viewer.getSceneRoot().getName())
				throw new IllegalArgumentException("empty pick path must start at scene root!");
			if (emptyPickPath.getFirstElement() != viewer.getSceneRoot()) {
				LoggingSystem.getLogger(this).warning("Strange situation: same names but different scene roots");
			}
			this.emptyPickPath = emptyPickPath;
		} else {
			this.emptyPickPath = new SceneGraphPath();
			this.emptyPickPath.push(viewer.getSceneRoot());
		}
	}

    public RenderTrigger getRenderTrigger() {
        return renderTrigger;
    }

	public Object getKey() {
		return KEY;
	}

}
