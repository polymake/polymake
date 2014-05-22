/**
   TrackdJNI is the class for connecting to the trackd native methods.  
*/

package de.jreality.devicedriver;

public final class TrackdJNI {
  
  // ???
  //public static final int TRACKER = 1;
  //public static final int CONTROLLER = 2;

  /**
   * The values for tracker_shmkey and controller_shmkey are defined in the trackd config file.
   *  
   * @param tracker_shmkey
   * @param controller_shmkey
   * @throws java.io.IOException
   */
  public TrackdJNI(int tracker_shmkey, int controller_shmkey)
         throws java.io.IOException {
    System.loadLibrary("JavaTrackdAPI");
    int[] status = {0};
    init_trackd_c(tracker_shmkey, controller_shmkey, status);
    if (status[0] != 0) {
      throw new java.io.IOException("Unable to connect to trackd using tracker " +
                                    "shared memory segment " + tracker_shmkey +
                                    " and controller shared memory segment " + 
                                    controller_shmkey + ": trackd API return status was " +
                                     + status[0]);
    }
  }

  private native void init_trackd_c(int tracker_shmkey,
                                    int controller_shmkey,
                                    int[] status);

  public native void getMatrix(float[] matrix, int which);

  public native int getButton(int which);

  public native float getValuator(int which);

  public native int getNumSensors();

  public native int getNumButtons();

  public native int getNumValuators();

}

