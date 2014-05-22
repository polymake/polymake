package de.jreality.junitutils;

import java.awt.Component;
import java.awt.Container;
import java.util.LinkedList;
import java.util.List;

import org.junit.Ignore;


@Ignore
public class GuiTestUtils {
	/** Has only static members.
	 */
	protected GuiTestUtils() {}
	
	public static class ComponentsFinder<T extends Component> {
		private Matcher<T> matcher;

		public ComponentsFinder(Matcher<T> decider) {
			this.matcher = decider;
		}

		public ComponentsFinder(final Class<T> classToken) {
			this.matcher = new Matcher<T>() {
				public T castWhenMatches(Component cp) {
					if (classToken.isInstance(cp)) {
						return classToken.cast(cp);
					} else {
						return null;
					}
				}
			};
		}
		public List<T> getComponents(Container container) {
			List<T> components = new LinkedList<T>();
			for (Component cp : container.getComponents()) {
				T element = matcher.castWhenMatches(cp);
				if (null != element) components.add(element);
				if (cp instanceof Container && 0 < ((Container) cp).getComponentCount()) {
					components.addAll(getComponents((Container) cp));
				}
			}
			return components;
		}

		public int getComponentsCount(Container container) {
			return getComponents(container).size();
		}
	}

	public static interface Matcher<T> {
		public T castWhenMatches(Component cp);
	}
}
