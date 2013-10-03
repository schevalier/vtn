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
import java.net.InetAddress;
import java.util.concurrent.atomic.AtomicLong;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.opendaylight.vtn.manager.internal.VTNManagerImpl;

/**
 * {@code ClusterEventId} describes an identifier of cluster cache event.
 *
 * <p>
 *   Cluster event ID, which is unique identifier in the cluster, is assigned
 *   to every cluster cache event which delivers arbitrary event to controllers
 *   in the cluster. A cluster event ID consists of the IP address of the
 *   controller which generated the event, and a long integer value unique
 *   in the controller.
 * </p>
 * <p>
 *   Although this class is public to other packages, this class does not
 *   provide any API. Applications other than VTN Manager must not use this
 *   class.
 * </p>
 */
public class ClusterEventId implements Serializable {
    /**
     * Version number for serialization.
     */
    private static final long serialVersionUID = 1962921552441279134L;

    /**
     * A character which separates fields in a string representation of
     * event ID.
     */
    public static final char  SEPARATOR = '-';

    /**
     * IP address of this controller.
     */
    private static InetAddress  localAddress;

    /**
     * Event identifier for the next allocation.
     */
    private static final AtomicLong  NEXT_EVENT_ID = new AtomicLong();

    /**
     * IP address of the controller which generated the event.
     */
    private final InetAddress  controllerAddress;

    /**
     * Event identifier.
     */
    private final long  eventId;

    /**
     * Set the IP address of this controller.
     *
     * <p>
     *   Note that this method is not synchronized.
     *   This method must be called while no VTN Manager service is running.
     * </p>
     *
     * @param ipaddr  The IP address of this controller.
     *                Specifying {@code null} results in undefined behavior.
     */
    public static void setLocalAddress(InetAddress ipaddr) {
        localAddress = ipaddr;
        Logger logger = LoggerFactory.getLogger(ClusterEventId.class);
        if (logger.isDebugEnabled()) {
            logger.debug("Local node address: {}", ipaddr.getHostAddress());
        }
    }

    /**
     * Return the IP address of this controller.
     *
     * @return  The IP address of this controller.
     */
    private static InetAddress getLocalAddress() {
        InetAddress ipaddr = localAddress;
        if (ipaddr == null) {
            // This code is only for unit test.
            ipaddr = InetAddress.getLoopbackAddress();
        }

        return ipaddr;
    }

    /**
     * Construct a new cluster event identifier.
     *
     * <p>
     *   This constructor uses the IP address of this controller, and assigns
     *   a new event ID.
     * </p>
     */
    public ClusterEventId() {
        this(getLocalAddress(), NEXT_EVENT_ID.getAndIncrement());
    }

    /**
     * Construct a new cluster event identifier specifying a pair of IP address
     * and event ID.
     *
     * @param addr   IP address of the controller.
     * @param id     The event ID.
     */
    public ClusterEventId(InetAddress addr, long id) {
        controllerAddress = addr;
        eventId = id;
    }

    /**
     * Return the IP address of the controller which generated the event.
     *
     * @return  A controller's IP address in this event ID.
     */
    public InetAddress getControllerAddress() {
        return controllerAddress;
    }

    /**
     * Return an event identifier.
     *
     * @return  An event identifier in this event ID.
     */
    public long getEventId() {
        return eventId;
    }

    /**
     * Determine whether this event ID is generated by the local cluster node
     * or not.
     *
     * @return  {@code true} is returned if this event ID is generated by the
     *          local cluster node. Otherwise {@code false} is returned.
     */
    public boolean isLocal() {
        return controllerAddress.equals(getLocalAddress());
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
        if (!(o instanceof ClusterEventId)) {
            return false;
        }

        ClusterEventId evid = (ClusterEventId)o;
        return (controllerAddress.equals(evid.controllerAddress) &&
                eventId == evid.eventId);
    }


    /**
     * Return the hash code of this object.
     *
     * @return  The hash code.
     */
    @Override
    public int hashCode() {
        return controllerAddress.hashCode() ^ VTNManagerImpl.hashCode(eventId);
    }

    /**
     * Return a string representation of this object.
     *
     * @return  A string representation of this object.
     */
    @Override
    public String toString() {
        StringBuilder builder =
            new StringBuilder(controllerAddress.getHostAddress());
        builder.append(SEPARATOR).append(eventId);
        return builder.toString();
    }
}