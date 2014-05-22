package de.jreality.audio;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.scene.AudioSource;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.tools.ActionTool;
import de.jreality.tools.DraggingTool;

/**
 * Basic test to check spatial audio.
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 *
 */
public class TestVR {

	public static void main(String[] args) throws Exception {

		SceneGraphComponent cmp = new SceneGraphComponent();
		DraggingTool dragtool = new DraggingTool();


		//		Input wavWaterdrops = Input.getInput("/Users/tim/Documents/workspace/jreality/data/Gun1.wav");
		//		final AudioSource s1 = new CachedAudioInputStreamSource("wavnode", wavWaterdrops, true);
		final ElectricBass s1 = new ElectricBass("Bass");
		final SceneGraphComponent cmp1 = new SceneGraphComponent();
		cmp1.setGeometry(new Sphere());
		MatrixBuilder.euclidean().translate(-4, 0, 0).assignTo(cmp1);
		cmp.addChild(cmp1);
		cmp1.setAudioSource(s1);
		ActionTool at1 = new ActionTool("PanelActivation");
		at1.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if (s1.getState() == AudioSource.State.RUNNING) {
					s1.noteOff();
					s1.pause();
				}
				else {
					int pitch = (int)(cmp1.getTransformation().getMatrix()[11]*4) +20;
					System.out.println("pitch raw "+pitch);
					pitch = Math.min(100,pitch);
					pitch = Math.max(pitch,10);
					System.out.println("pitch map "+pitch);
					s1.noteOn(pitch, 90);
					s1.start();
				}
			}
		});
		OnOffTool ata = new OnOffTool() {
			@Override
			public void activate(ToolContext tc) {
				super.activate(tc);
				//System.out.println("active");
				int pitch = (int)(cmp1.getTransformation().getMatrix()[11]*3) +20;

				pitch = Math.min(100,pitch);
				pitch = Math.max(pitch,10);

				s1.noteOn(pitch, 90);
				//s1.start();
			}

			@Override
			public void deactivate(ToolContext tc) {
				super.deactivate(tc);
				//System.out.println("inactive");
				if (s1.getState() == AudioSource.State.RUNNING) {
					s1.noteOff();
					//s1.pause();
				}
			}
		};
		//cmp1.addTool(at1);
		cmp1.addTool(ata);
		cmp1.addTool(dragtool);
		s1.start();


		SceneGraphComponent cmp2 = new SceneGraphComponent();
		cmp.addChild(cmp2);

		final SynthSource sin = new SynthSource("wave", 44100) {
			float amplitude=0.03f;
			double frequency=440;
			@Override
			public float nextSample() {
				return amplitude * (float) Math.sin(2*Math.PI*index*frequency/sampleRate);
			}
		};

		cmp2.setGeometry(Primitives.icosahedron());
		MatrixBuilder.euclidean().translate(0, 0, 0).assignTo(cmp2);
		cmp2.setAudioSource(sin);
		ActionTool at2 = new ActionTool("PanelActivation");
		at2.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if (sin.getState() == AudioSource.State.RUNNING) sin.pause();
				else sin.start();
			}
		});
		cmp2.addTool(at2);
		cmp2.addTool(dragtool);
		sin.start();

		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addAudioSupport();
		v.addVRSupport();
		v.setPropertiesFile("TestVR.jrw");
		v.addContentSupport(ContentType.TerrainAligned);
		v.setContent(cmp);
		v.startup();
	}
	
	private static abstract class OnOffTool extends AbstractTool {
		OnOffTool() {
			super(InputSlot.getDevice("PrimaryAction"));
		}
	}
}
