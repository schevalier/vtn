/*
 * Copyright (c) 2013 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.vtn.manager.neutron;

import org.junit.Assert;

/**
 * Abstract base class for JUnit tests.
 */
public abstract class TestBase extends Assert {
    /**
     * An array of UUIDs generated by Neutron.
     */
    protected static final String[] NEUTRON_UUID_ARRAY
        = {"C387EB44-7832-49F4-B9F0-D30D27770883",
           "4790F3C1-AB34-4ABC-B7A5-C1B5C7202389",
           "52B1482F-A41E-409F-AC68-B04ACFD07779",
           "6F3FCFCF-C000-4879-84C5-19157CBD1F6A",
           "0D2206F8-B700-4F78-913D-9CE7A2D78473"};

    /**
     * An array of tenat IDs generated by Keystone.
     */
    protected static final String[] TENANT_ID_ARRAY
        = {"E6E005D3A24542FCB03897730A5150E2",
           "B37B50456AE848DF8F981058FDD3A63D",
           "3655F990CC5348F2A668F77896D9D017",
           "E20F7C4511144D9BAEB844A4964B60DA",
           "5FB6A2BB77714CE6BF33282283245705"};

    /**
     * An array of MAC Addresses.
     */
    protected static final String[] MAC_ADDR_ARRAY
        = {"EA:75:F6:3A:30:50",
           "20:C1:F2:C0:A5:BF",
           "33:C2:B0:C3:B4:40",
           "9B:D9:F0:C4:31:B1",
           "70:A8:A6:C2:FA:D2"};
}
