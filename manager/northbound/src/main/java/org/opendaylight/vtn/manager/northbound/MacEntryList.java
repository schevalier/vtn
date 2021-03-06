/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.vtn.manager.northbound;

import java.util.List;
import java.util.ArrayList;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;

import org.opendaylight.vtn.manager.MacAddressEntry;

/**
 * {@link MacEntryList} class describes a list of MAC address information.
 *
 * <p>
 *   This class is used to return a list of MAC address information learned
 *   inside the MAC address table in the vBridge to REST client.
 * </p>
 */
@JsonSerialize(include = JsonSerialize.Inclusion.NON_NULL)
@XmlRootElement(name = "macentries")
@XmlAccessorType(XmlAccessType.NONE)
public class MacEntryList {
    /**
     * A list of {@link MacEntry} instances.
     *
     * <ul>
     *   <li>
     *     This element contains 0 or more {@link MacEntry} instances which
     *     represent MAC address information.
     *   </li>
     * </ul>
     */
    @XmlElement(name = "macentry")
    private List<MacEntry>  entries;

    /**
     * Default constructor.
     */
    public MacEntryList() {
    }

    /**
     * Construct a list of MAC address table entries.
     *
     * @param list  A list of MAC address table entries.
     */
    public MacEntryList(List<MacAddressEntry> list) {
        if (list != null) {
            List<MacEntry> elist = new ArrayList<MacEntry>();
            for (MacAddressEntry entry: list) {
                elist.add(new MacEntry(entry));
            }
            entries = elist;
        }
    }

    /**
     * Return a list of MAC address table entries.
     *
     * @return A list of MAC address table entries.
     */
    List<MacEntry> getList() {
        return entries;
    }

    /**
     * Determine whether the given object is identical to this object.
     *
     * @param o  An object to be compared.
     * @return   {@code true} if identical. Otherwise {@code false}.
     */
    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof MacEntryList)) {
            return false;
        }

        List<MacEntry> list = ((MacEntryList)o).entries;
        if (entries == null || entries.isEmpty()) {
            return (list == null || list.isEmpty());
        }

        return entries.equals(list);
    }

    /**
     * Return the hash code of this object.
     *
     * @return  The hash code.
     */
    @Override
    public int hashCode() {
        int h = 0;
        if (entries != null && !entries.isEmpty()) {
            h ^= entries.hashCode();
        }

        return h;
    }
}
