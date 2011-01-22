/*
 * Copyright (c) 2008 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

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
        NiteMocap mocap = new NiteMocap();
        System.err.println("done.");
        for (int i = 0; i < 1000; i++) {
//System.err.println("here: " + i);
            mocap.sense();
            float[][][] r = mocap.get();
            for (int j = 0; j < 1; j++) {
                if (r[j][0][8] != 0) {
                    for (int k = 0; k < 16; k++) {
                        for (int l = 0; l < 9; l++) {
                            if (r[j][k][0] != 0 && r[j][k][4] != 0) {
                                if (l == 0) {
                                    System.err.print("[" + j + "][" + k + "] ");
                                }
                                System.err.printf("%.2f, ", r[j][k][l]);
                            }
                        }
                        System.err.println();
                    }
                    System.err.println();
//                } else {
//                    System.err.println("not detect: " + j);
                }
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
