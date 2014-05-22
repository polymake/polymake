from de.jreality.plugin import JRViewerUtility
from de.jreality.geometry import Primitives

content = JRViewerUtility.getContentPlugin(C);
content.setContent(Primitives.regularAnnulus(100, 0.5, 0.1));
