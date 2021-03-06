/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.vtn.manager.internal.cluster;

import java.util.List;

import org.opendaylight.controller.forwardingrulesmanager.FlowEntry;

import org.opendaylight.vtn.manager.internal.ClusterFlowModTask;
import org.opendaylight.vtn.manager.internal.ClusterFlowRemoveTask;
import org.opendaylight.vtn.manager.internal.VTNManagerImpl;

/**
 * {@code FlowRemoveEvent} describes an cluster event object which directs
 * remote cluster node to uninstall the given flow entries.
 *
 * <p>
 *   Although this class is public to other packages, this class does not
 *   provide any API. Applications other than VTN Manager must not use this
 *   class.
 * </p>
 */
public class FlowRemoveEvent extends FlowModEvent {
    /**
     * Version number for serialization.
     */
    private static final long serialVersionUID = -4133633144252779271L;

    /**
     * Construct a new flow uninstall event.
     *
     * @param entries  List of flow entries to be uninstalled.
     */
    public FlowRemoveEvent(List<FlowEntry> entries) {
        super(entries);
    }

    /**
     * Create a flow mod task to modify the given flow entry.
     *
     * @param mgr   VTN Manager service.
     * @param fent  A flow entry to be modified.
     * @return  A flow mod task to remove the given flow entry.
     */
    protected ClusterFlowModTask createTask(VTNManagerImpl mgr,
                                            FlowEntry fent) {
        return new ClusterFlowRemoveTask(mgr, fent);
    }
}
