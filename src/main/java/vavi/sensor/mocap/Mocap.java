/*
 * Copyright (c) 2008 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sensor.mocap;


/**
 * Mocap. 
 *
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 080715 nsano initial version <br>
 */
public interface Mocap<T> {

    /** */
    int sense();

    /** */
    T get();
}

/* */
