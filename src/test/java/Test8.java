/*
 * Copyright (c) 2008 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

import vavi.sensor.mocap.Mocap;
import vavi.sensor.mocap.macbook.NiteMocap;


/**
 * Test8.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 070202 initial version <br>
 */
public class Test8 {

    public void test01() throws Exception {
        System.err.println("initializing...");
        Mocap<float[][][]> mocap = new NiteMocap();
        System.err.println("done.");
        for (int i = 0; i < 1000; i++) {
//System.err.println("here: " + i);
            mocap.sense();
            float[][][] r = mocap.get();
System.err.println("here: " + r[0][0].length + ", " + r[0].length + ", " + r.length);
            for (int j = 0; j < 15; j++) {
                System.err.print("[" + j + "]");
                for (int k = 0; k < 16; k++) {
                    System.err.print("[" + k + "] ");
                    for (int l = 0; l < 9; l++) {
                        System.err.print(r[j][k][l] + ", ");
                    }
                    System.err.println();
                }
                System.err.println();
            }
            Thread.sleep(100);
        }
    }

    public static void main(String[] args) throws Exception {
        Test8 test = new Test8();
        test.test01();
    }
}

/* */
