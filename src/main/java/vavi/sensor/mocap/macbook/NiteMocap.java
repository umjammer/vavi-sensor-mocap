/*
 * Copyright (c) 2009 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sensor.mocap.macbook;

import vavi.sensor.mocap.Mocap;


/**
 * OpenNI NITE. 
 *
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 2009/08/24 nsano initial version <br>
 */
public class NiteMocap implements Mocap<float[][][]> {

    /** */
    private native int init();
    
    /** */
    public native int sense();

    /** */
    private native void destroy();
    
    public NiteMocap() {
        int r = init();
        if (r != 0) {
            // 131076 Open failed: File not found!
            throw new IllegalStateException("error: " + r);
        }
    }

    protected void finalize() throws Throwable {
        destroy();
    }

    public native float[][][] get();

    static {
        try {
            System.loadLibrary("NiteWrapper");
        } catch (Exception e) {
            throw new IllegalStateException(e);
        }
    }
}

/* */
