/*
 * Copyright (c) 2013 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.vtn.manager.internal.cluster;

import java.io.Serializable;

import org.slf4j.Logger;

import org.opendaylight.vtn.manager.internal.VTNManagerImpl;

/**
 * Base class of cluster events which is broadcasted to cluster nodes.
 *
 * <p>
 *   An instance of this class is used to raise arbitrary event to remote
 *   cluster nodes via InfiniSpan cache.
 * </p>
 * <p>
 *   Although this class is public to other packages, this class does not
 *   provide any API. Applications other than VTN Manager must not use this
 *   class.
 * </p>
 */
public abstract class ClusterEvent implements Serializable {
    /**
     * Version number for serialization.
     */
    private static final long serialVersionUID = -7483110677313880843L;

    /**
     * Construct a new cluster event object.
     */
    protected ClusterEvent() {
    }

    /**
     * Invoked when a cluster event has been received from remote cluster node.
     *
     * @param mgr    VTN Manager service.
     * @param local  {@code true} if this event is generated by the local node.
     *               {@code false} if this event is generated by remote cluster
     *               node.
     */
    public final void received(final VTNManagerImpl mgr, final boolean local) {
        if (isSingleThreaded(local)) {
            // This event must be delivered on the VTN task thread.
            Runnable r = new Runnable() {
                @Override
                public void run() {
                    eventReceived(mgr, local);
                }
            };
            mgr.postTask(r);
        } else {
            eventReceived(mgr, local);
        }
    }

    /**
     * Invoked when a cluster event has been received from remote cluster node.
     *
     * @param mgr    VTN Manager service.
     * @param local  {@code true} if this event is generated by the local node.
     *               {@code false} if this event is generated by remote cluster
     *               node.
     */
    protected abstract void eventReceived(VTNManagerImpl mgr, boolean local);

    /**
     * Record a trace log which indicates that a cluster event has been
     * received from remote node.
     *
     * @param mgr     VTN Manager service.
     * @param logger  A logger instance.
     * @param key     A cluster event key associated with this event.
     */
    public abstract void traceLog(VTNManagerImpl mgr, Logger logger,
                                  ClusterEventId key);

    /**
     * Determine whether this event should be delivered on the VTN task thread
     * or not.
     *
     * @param local  {@code true} if this event is generated by the local node.
     *               {@code false} if this event is generated by remote cluster
     *               node.
     * @return  {@code true} is returned if this event should be delivered
     *          on the VTN task thread. Otherwise {@code false} is returned.
     */
    public abstract boolean isSingleThreaded(boolean local);
}
