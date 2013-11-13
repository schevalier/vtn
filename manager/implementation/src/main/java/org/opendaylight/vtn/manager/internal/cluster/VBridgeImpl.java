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
import java.io.ObjectInputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;
import java.util.List;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.opendaylight.vtn.manager.IVTNManagerAware;
import org.opendaylight.vtn.manager.PortMap;
import org.opendaylight.vtn.manager.PortMapConfig;
import org.opendaylight.vtn.manager.VBridge;
import org.opendaylight.vtn.manager.VBridgeConfig;
import org.opendaylight.vtn.manager.VBridgeIfPath;
import org.opendaylight.vtn.manager.VBridgePath;
import org.opendaylight.vtn.manager.VInterface;
import org.opendaylight.vtn.manager.VInterfaceConfig;
import org.opendaylight.vtn.manager.VNodeState;
import org.opendaylight.vtn.manager.VTNException;
import org.opendaylight.vtn.manager.VTenantConfig;
import org.opendaylight.vtn.manager.VTenantPath;
import org.opendaylight.vtn.manager.VlanMap;
import org.opendaylight.vtn.manager.VlanMapConfig;
import org.opendaylight.vtn.manager.internal.ActionList;
import org.opendaylight.vtn.manager.internal.IVTNResourceManager;
import org.opendaylight.vtn.manager.internal.MacAddressTable;
import org.opendaylight.vtn.manager.internal.MapType;
import org.opendaylight.vtn.manager.internal.PacketContext;
import org.opendaylight.vtn.manager.internal.VTNConfig;
import org.opendaylight.vtn.manager.internal.VTNFlowDatabase;
import org.opendaylight.vtn.manager.internal.VTNManagerImpl;
import org.opendaylight.vtn.manager.internal.VTNThreadData;

import org.opendaylight.controller.sal.core.Edge;
import org.opendaylight.controller.sal.core.Node;
import org.opendaylight.controller.sal.core.NodeConnector;
import org.opendaylight.controller.sal.core.Path;
import org.opendaylight.controller.sal.core.UpdateType;
import org.opendaylight.controller.sal.match.Match;
import org.opendaylight.controller.sal.packet.Ethernet;
import org.opendaylight.controller.sal.packet.PacketResult;
import org.opendaylight.controller.sal.routing.IRouting;
import org.opendaylight.controller.sal.topology.TopoEdgeUpdate;
import org.opendaylight.controller.sal.utils.NetUtils;
import org.opendaylight.controller.sal.utils.Status;
import org.opendaylight.controller.sal.utils.StatusCode;
import org.opendaylight.controller.switchmanager.ISwitchManager;

/**
 * Implementation of virtual layer 2 bridge.
 *
 * <p>
 *   Although this class is public to other packages, this class does not
 *   provide any API. Applications other than VTN Manager must not use this
 *   class.
 * </p>
 */
public final class VBridgeImpl implements Serializable {
    /**
     * Version number for serialization.
     */
    private static final long serialVersionUID = -8520771325355548289L;

    /**
     * Logger instance.
     */
    private static final Logger  LOG =
        LoggerFactory.getLogger(VBridgeImpl.class);

    /**
     * Default interval of MAC address table aging.
     */
    private static final int  DEFAULT_AGE_INTERVAL = 600;

    /**
     * Maximum interval of MAC address table aging.
     */
    private static final int  MAX_AGE_INTERVAL = 1000000;

    /**
     * Minimum interval of MAC address table aging.
     */
    private static final int  MIN_AGE_INTERVAL = 10;

    /**
     * Pseudo node identifier which indicates that the node is unspecified.
     */
    private static final String  NODEID_ANY = "ANY";

    /**
     * Virtual tenant which includes this bridge.
     */
    private transient VTenantImpl  parent;

    /**
     * Path to the bridge.
     */
    private transient VBridgePath  bridgePath;

    /**
     * Configuration for the bridge.
     */
    private VBridgeConfig  bridgeConfig;

    /**
     * Attached virtual interfaces.
     */
    private final Map<String, VBridgeIfImpl> vInterfaces =
        new TreeMap<String, VBridgeIfImpl>();

    /**
     * VLAN mappings applied to this bridge.
     */
    private final Map<String, VlanMapImpl> vlanMaps =
        new TreeMap<String, VlanMapImpl>();

    /**
     * VLAN mappings indexed by VLAN ID.
     */
    private transient Map<Short, VlanIdMap> vlanIdIndex =
        new TreeMap<Short, VlanIdMap>();

    /**
     * Read write lock to synchronize per-bridge resources.
     */
    private transient ReentrantReadWriteLock  rwLock =
        new ReentrantReadWriteLock();

    /**
     * Construct a virtual bridge instance.
     *
     * @param vtn   The virtual tenant to which a new bridge belongs.
     * @param name  The name of the bridge.
     * @param bconf Configuration for the bridge.
     * @throws VTNException  An error occurred.
     */
    VBridgeImpl(VTenantImpl vtn, String name, VBridgeConfig bconf)
        throws VTNException {
        VBridgeConfig cf = resolve(bconf);
        checkConfig(cf);
        bridgeConfig = cf;
        setPath(vtn, name);
    }

    /**
     * Set virtual bridge path.
     *
     * @param vtn   Virtual tenant which includes this bridge.
     * @param name  The name of this bridge.
     */
    void setPath(VTenantImpl vtn, String name) {
        parent = vtn;
        bridgePath = new VBridgePath(vtn.getName(), name);

        // Set this bridge as parent of interfaces.
        for (Map.Entry<String, VBridgeIfImpl> entry: vInterfaces.entrySet()) {
            String iname = entry.getKey();
            VBridgeIfImpl vif = entry.getValue();
            vif.setPath(this, iname);
        }

        // Initialize VLAN mapping path.
        for (Map.Entry<String, VlanMapImpl> entry: vlanMaps.entrySet()) {
            String mapId = entry.getKey();
            VlanMapImpl vmap = entry.getValue();
            vmap.setPath(bridgePath, mapId);
        }
    }

    /**
     * Return the name of the container to which the bridge belongs.
     *
     * @return  The name of the container.
     */
    String getContainerName() {
        return parent.getContainerName();
    }

    /**
     * Return the name of the tenant to which the bridge belongs.
     *
     * @return  The name of the container.
     */
    String getTenantName() {
        return parent.getName();
    }

    /**
     * Return a virtual tenant instance which contains this bridge.
     *
     * @return  A virtual tenant instance.
     */
    VTenantImpl getTenant() {
        return parent;
    }

    /**
     * Return the name of the bridge.
     *
     * @return  The name of the bridge.
     */
    String getName() {
        return bridgePath.getBridgeName();
    }

    /**
     * Return path to this bridge.
     *
     * @return  Path to the bridge.
     */
    VBridgePath getPath() {
        return bridgePath;
    }

    /**
     * Return the state of the bridge.
     *
     * @param mgr  VTN Manager service.
     * @return  The state of the bridge.
     */
    VNodeState getState(VTNManagerImpl mgr) {
        VBridgeState bst = getBridgeState(mgr);
        return bst.getState();
    }

    /**
     * Return information about the bridge.
     *
     * @param mgr  VTN Manager service.
     * @return  Information about the bridge.
     */
    VBridge getVBridge(VTNManagerImpl mgr) {
        return getVBridge(mgr, getName(), getVBridgeConfig());
    }

    /**
     * Return bridge configuration.
     *
     * @return  Configuration for the bridge.
     */
    synchronized VBridgeConfig getVBridgeConfig() {
        return bridgeConfig;
    }

    /**
     * Set bridge configuration.
     *
     * @param mgr    VTN Manager service.
     * @param bconf  Bridge configuration.
     * @param all    If {@code true} is specified, all attributes of the
     *               bridge are modified. In this case, {@code null} in
     *               {@code bconf} is interpreted as default value.
     *               If {@code false} is specified, an attribute is not
     *               modified if its value in {@code bconf} is {@code null}.
     * @return  {@code true} if the configuration is actually changed.
     *          Otherwise {@code false}.
     * @throws VTNException  An error occurred.
     */
    synchronized boolean setVBridgeConfig(VTNManagerImpl mgr,
                                          VBridgeConfig bconf, boolean all)
        throws VTNException {
        VBridgeConfig cf = (all) ? resolve(bconf) : merge(bconf);
        if (cf.equals(bridgeConfig)) {
            return false;
        }

        checkConfig(cf);
        bridgeConfig = cf;
        String name = bridgePath.getBridgeName();
        VBridge vbridge = getVBridge(mgr, name, cf);
        VBridgeEvent.changed(mgr, bridgePath, vbridge, true);

        initMacTableAging(mgr);
        return true;
    }

    /**
     * Add a new virtual interface to this bridge.
     *
     * @param mgr    VTN Manager service.
     * @param path   Path to the bridge.
     * @param iconf  Interface configuration.
     * @throws VTNException  An error occurred.
     */
    void addInterface(VTNManagerImpl mgr, VBridgeIfPath path,
                      VInterfaceConfig iconf) throws VTNException {
        // Ensure the given interface name is valid.
        String ifName = path.getInterfaceName();
        VTNManagerImpl.checkName("Interface", ifName);

        if (iconf == null) {
            Status status = VTNManagerImpl.
                argumentIsNull("Interface configuration");
            throw new VTNException(status);
        }

        VBridgeIfImpl vif = new VBridgeIfImpl(this, ifName, iconf);
        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            VBridgeIfImpl old = vInterfaces.put(ifName, vif);
            if (old != null) {
                vInterfaces.put(ifName, old);
                String msg = ifName + ": Interface name already exists";
                throw new VTNException(StatusCode.CONFLICT, msg);
            }

            VInterface viface = vif.getVInterface(mgr);
            VBridgeIfEvent.added(mgr, path, viface);
            updateState(mgr);
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Change configuration of existing virtual interface.
     *
     * @param mgr    VTN Manager service.
     * @param path   Path to the interface.
     * @param iconf  Interface configuration.
     * @param all    If {@code true} is specified, all attributes of the
     *               interface are modified. In this case, {@code null} in
     *               {@code iconf} is interpreted as default value.
     *               If {@code false} is specified, an attribute is not
     *               modified if its value in {@code iconf} is {@code null}.
     * @return  {@code true} is returned only if the interface configuration is
     *          actually changed.
     * @throws VTNException  An error occurred.
     */
    boolean modifyInterface(VTNManagerImpl mgr, VBridgeIfPath path,
                            VInterfaceConfig iconf, boolean all)
        throws VTNException {
        if (iconf == null) {
            Status status = VTNManagerImpl.
                argumentIsNull("Interface configuration");
            throw new VTNException(status);
        }

        // Write lock is needed because this code determines the state of
        // this bridge by scanning interfaces.
        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            VBridgeIfImpl vif = getInterfaceImpl(path);
            if (!vif.setVInterfaceConfig(mgr, iconf, all)) {
                return false;
            }
            updateState(mgr);
            return true;
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Remove the specified virtual interface.
     *
     * @param mgr   VTN Manager service.
     * @param path  Path to the interface.
     * @throws VTNException  An error occurred.
     */
    void removeInterface(VTNManagerImpl mgr, VBridgeIfPath path)
        throws VTNException {
        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            String ifName = path.getInterfaceName();
            if (ifName == null) {
                Status status = VTNManagerImpl.
                    argumentIsNull("Interface name");
                throw new VTNException(status);
            }

            VBridgeIfImpl vif = vInterfaces.remove(ifName);
            if (vif == null) {
                Status status = interfaceNotFound(ifName);
                throw new VTNException(status);
            }

            vif.destroy(mgr, true);
            updateState(mgr);
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Return a list of all bridge interface information.
     *
     * @param mgr   VTN Manager service.
     * @return  A list of bridge interface information.
     */
    List<VInterface> getInterfaces(VTNManagerImpl mgr) {
        ArrayList<VInterface> list = new ArrayList<VInterface>();
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            for (VBridgeIfImpl vif: vInterfaces.values()) {
                list.add(vif.getVInterface(mgr));
            }
            list.trimToSize();
        } finally {
            rdlock.unlock();
        }

        return list;
    }

    /**
     * Return information about the virtual bridge interface associated with
     * the given name.
     *
     * @param mgr   VTN Manager service.
     * @param path  Path to the interface.
     * @return  The virtual interface information associated with the given
     *          name.
     * @throws VTNException  An error occurred.
     */
    VInterface getInterface(VTNManagerImpl mgr, VBridgeIfPath path)
        throws VTNException {
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            VBridgeIfImpl vif = getInterfaceImpl(path);
            return vif.getVInterface(mgr);
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Add VLAN mapping to this bridge.
     *
     * @param mgr     VTN Manager serivce.
     * @param vlconf  Configuration for the VLAN mapping.
     * @return  Information about the added VLAN mapping is returned.
     * @throws VTNException  An error occurred.
     */
    VlanMap addVlanMap(VTNManagerImpl mgr, VlanMapConfig vlconf)
        throws VTNException {
        if (vlconf == null) {
            Status status = VTNManagerImpl.
                argumentIsNull("VLAN map configiguration");
            throw new VTNException(status);
        }

        short vlan = vlconf.getVlan();
        VTNManagerImpl.checkVlan(vlan);

        // Create ID for this VLAN mapping.
        Node node = vlconf.getNode();
        StringBuilder idBuilder = new StringBuilder();
        if (node == null) {
            // Node is unspecified.
            idBuilder.append(NODEID_ANY);
        } else {
            VTNManagerImpl.checkNode(node);
            idBuilder.append(node.getType()).append('-').
                append(node.getNodeIDString());
        }

        idBuilder.append('.').append((int)vlan);

        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            // Create a VLAN mapping instance.
            String id = idBuilder.toString();
            VlanMapImpl vmap = new VlanMapImpl(bridgePath, id, vlconf);

            // Update VLAN ID index.
            boolean needreg = addVlanIdIndex(vmap);
            if (needreg) {
                // Ensure that the specified VLAN ID is not mapped to another
                // bridge.
                IVTNResourceManager resMgr = mgr.getResourceManager();
                String anotherBridge =
                    resMgr.registerVlanMap(getContainerName(), bridgePath,
                                           vlan);
                if (anotherBridge != null) {
                    removeVlanIdIndex(vlconf);
                    String msg = "VLAN ID " + vlan + " is mapped to " +
                        anotherBridge;
                    throw new VTNException(StatusCode.CONFLICT, msg);
                }
            }

            vmap.initState(mgr);
            vlanMaps.put(id, vmap);

            VlanMap vlmap = new VlanMap(id, node, vlan);
            VlanMapEvent.added(mgr, bridgePath, vlmap);
            if (vmap.isValid(mgr.getStateDB())) {
                updateState(mgr);
            } else {
                setState(mgr, VNodeState.DOWN);
            }
            return vlmap;
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Remove VLAN mapping from this bridge.
     *
     * @param mgr     VTN Manager serivce.
     * @param mapId   The identifier of the VLAN mapping.
     * @throws VTNException  An error occurred.
     */
    void removeVlanMap(VTNManagerImpl mgr, String mapId)
        throws VTNException {
        if (mapId == null) {
            Status status = VTNManagerImpl.argumentIsNull("Mapping ID");
            throw new VTNException(status);
        }

        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            VlanMapImpl vmap = vlanMaps.remove(mapId);
            if (vmap == null) {
                Status status = vlanMapNotFound(mapId);
                throw new VTNException(status);
            }

            VlanMapConfig vlconf = vmap.getVlanMapConfig();
            short vlan = vlconf.getVlan();
            boolean unmapped = removeVlanIdIndex(vlconf);
            Node node = vlconf.getNode();
            if (unmapped) {
                // Flush MAC address table associated with this VLAN map.
                // Although this may flush entries associated with the port
                // mapping, it should never cause any problem.
                MacAddressTable table = mgr.getMacAddressTable(bridgePath);
                table.flush(node, vlan);

                // Unregister VLAN mapping.
                IVTNResourceManager resMgr = mgr.getResourceManager();
                resMgr.unregisterVlanMap(vlan);
            }

            VlanMap vlmap = new VlanMap(mapId, node, vlan);
            vmap.destroy(mgr, bridgePath, vlmap, true);
            updateState(mgr);
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Return a list of VLAN mappings in the bridge.
     *
     * @return  A list of VLAN mapping information.
     */
    List<VlanMap> getVlanMaps() {
        ArrayList<VlanMap> list = new ArrayList<VlanMap>();
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            for (Map.Entry<String, VlanMapImpl> entry: vlanMaps.entrySet()) {
                String id = entry.getKey();
                VlanMapImpl vmap = entry.getValue();
                VlanMapConfig vlconf = vmap.getVlanMapConfig();
                VlanMap vlmap = new VlanMap(id, vlconf.getNode(),
                                            vlconf.getVlan());
                list.add(vlmap);
            }
        } finally {
            rdlock.unlock();
        }

        return list;
     }

    /**
     * Return information about the specified VLAN mapping.
     *
     * @param mapId  The identifier of the VLAN mapping.
     * @return  VLAN mapping information associated with the given ID.
     * @throws VTNException  An error occurred.
     */
    VlanMap getVlanMap(String mapId) throws VTNException {
        if (mapId == null) {
            Status status = VTNManagerImpl.argumentIsNull("Mapping ID");
            throw new VTNException(status);
        }

        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            VlanMapImpl vmap = vlanMaps.get(mapId);
            if (vmap == null) {
                Status status = vlanMapNotFound(mapId);
                throw new VTNException(status);
            }

            VlanMapConfig vlconf = vmap.getVlanMapConfig();
            VlanMap vlmap = new VlanMap(mapId, vlconf.getNode(),
                                        vlconf.getVlan());
            return vlmap;
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Return information about the VLAN mapping associated with the
     * specified VLAN mapping configuration.
     *
     * @param vlconf  VLAN mapping configuration.
     * @return  VLAN mapping information associated with the given VLAN mapping
     *          configuration.
     * @throws VTNException  An error occurred.
     */
    VlanMap getVlanMap(VlanMapConfig vlconf) throws VTNException {
        if (vlconf == null) {
            Status status = VTNManagerImpl.
                argumentIsNull("VLAN map configiguration");
            throw new VTNException(status);
        }

        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            short vlan = vlconf.getVlan();
            VlanIdMap idmap = vlanIdIndex.get(vlan);
            if (idmap != null) {
                Node node = vlconf.getNode();
                VlanMapImpl vmap = idmap.get(node);
                if (vmap != null) {
                    assert vlconf.equals(vmap.getVlanMapConfig());
                    return new VlanMap(vmap.getMapId(), node, vlan);
                }
            }
        } finally {
            rdlock.unlock();
        }

        String msg = "VLAN mapping does not exist";
        throw new VTNException(StatusCode.NOTFOUND, msg);
    }

    /**
     * Return the port mapping configuration applied to the specified virtual
     * bridge interface.
     *
     * @param mgr   VTN Manager service.
     * @param path  Path to the bridge interface.
     * @return  Port mapping information.
     *          {@code null} is returned if port mapping is not configured.
     * @throws VTNException  An error occurred.
     */
    PortMap getPortMap(VTNManagerImpl mgr, VBridgeIfPath path)
        throws VTNException {
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            VBridgeIfImpl vif = getInterfaceImpl(path);
            return vif.getPortMap(mgr);
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Create or destroy mapping between the physical switch port and the
     * virtual bridge interface.
     *
     * @param mgr     VTN Manager service.
     * @param path    Path to the bridge interface.
     * @param pmconf  Port mapping configuration to be set.
     *                If {@code null} is specified, port mapping on the
     *                specified interface is destroyed.
     * @throws VTNException  An error occurred.
     */
    void setPortMap(VTNManagerImpl mgr, VBridgeIfPath path,
                    PortMapConfig pmconf) throws VTNException {
        // Acquire write lock to serialize port mapping change.
        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            VBridgeIfImpl vif = getInterfaceImpl(path);
            VNodeState ifState = vif.setPortMap(mgr, pmconf);
            if (vif.isEnabled()) {
                if (ifState == VNodeState.DOWN) {
                    setState(mgr, VNodeState.DOWN);
                } else {
                    updateState(mgr);
                }
            }
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Resume the virtual L2 bridge.
     *
     * <p>
     *   This method is called just after this bridge is instantiated from
     *   the configuration file.
     * </p>
     *
     * @param mgr   VTN Manager service.
     */
    void resume(VTNManagerImpl mgr) {
        VNodeState state = VNodeState.UNKNOWN;
        IVTNResourceManager resMgr = mgr.getResourceManager();
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
        String containerName = getContainerName();

        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            // Resume virtual interfaces.
            for (VBridgeIfImpl vif: vInterfaces.values()) {
                VNodeState s = vif.resume(mgr, state);
                if (vif.isEnabled()) {
                    state = s;
                }
            }

            for (VlanMapImpl vmap: vlanMaps.values()) {
                // Register VLAN mappings to the global resource manager.
                VlanMapConfig vlconf = vmap.getVlanMapConfig();
                short vlan = vlconf.getVlan();
                resMgr.registerVlanMap(containerName, bridgePath, vlan);

                // Resume the VLAN mappings.
                state = vmap.resume(mgr, state);
            }

            VBridgeState bst = getBridgeState(db);
            state = bst.setState(state);
            if (bst.isDirty()) {
                db.put(bridgePath, bst);
            }
            if (LOG.isDebugEnabled()) {
                LOG.debug("{}:{}: Resumed bridge: state={}",
                          containerName, bridgePath, state);
            }

            // Create a MAC address table for this bridge.
            initMacTableAging(mgr);
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Initialize MAC address table aging.
     *
     * @param mgr  VTN Manager service.
     */
    synchronized void initMacTableAging(VTNManagerImpl mgr) {
        int age = bridgeConfig.getAgeInterval();
        MacAddressTable table = mgr.getMacAddressTable(bridgePath);
        if (table == null) {
            mgr.addMacAddressTable(bridgePath, age);
        } else {
            table.setAgeInterval(age);
        }
    }

    /**
     * Invoked when a node is added, removed, or changed.
     *
     * @param mgr   VTN Manager service.
     * @param node  Node being updated.
     * @param type  Type of update.
     */
    void notifyNode(VTNManagerImpl mgr, Node node, UpdateType type) {
        VNodeState state = VNodeState.UNKNOWN;
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();

        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            boolean doflush = false;

            for (VBridgeIfImpl vif: vInterfaces.values()) {
                if (!doflush && type == UpdateType.REMOVED) {
                    PortMapConfig pmconf = vif.getPortMapConfig();
                    doflush = (pmconf != null &&
                               pmconf.getNode().equals(node));
                }

                VNodeState s = vif.notifyNode(mgr, db, state, node, type);
                if (vif.isEnabled()) {
                    if (LOG.isTraceEnabled()) {
                        LOG.trace("{}:{}: notifyNode(if): {} -> {}",
                                  getContainerName(), bridgePath, state, s);
                    }
                    state = s;
                }
            }

            for (VlanMapImpl vmap: vlanMaps.values()) {
                if (!doflush && type == UpdateType.REMOVED) {
                    VlanMapConfig vlconf = vmap.getVlanMapConfig();
                    Node vmnode = vlconf.getNode();
                    doflush = (vmnode == null || vmnode.equals(node));
                }

                VNodeState s = vmap.notifyNode(mgr, db, state, node, type);
                if (LOG.isTraceEnabled()) {
                    LOG.trace("{}:{}: notifyNode(vmap): {} -> {}",
                              getContainerName(), bridgePath, state, s);
                }
                state = s;
            }
            setState(mgr, state);

            if (doflush) {
                // Flush MAC address table entries associated with the
                // removed node.
                MacAddressTable table = mgr.getMacAddressTable(bridgePath);
                table.flush(node);
            }
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * This method is called when some properties of a node connector are
     * added/deleted/changed.
     *
     * @param mgr     VTN Manager service.
     * @param nc      Node connector being updated.
     * @param pstate  The state of the node connector.
     * @param type    Type of update.
     */
    void notifyNodeConnector(VTNManagerImpl mgr, NodeConnector nc,
                             VNodeState pstate, UpdateType type) {
        VNodeState state = VNodeState.UNKNOWN;
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();

        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            boolean doflush = false;

            for (VBridgeIfImpl vif: vInterfaces.values()) {
                if (!doflush && pstate != VNodeState.UP) {
                    NodeConnector mapped = vif.getMappedPort(mgr);
                    doflush = nc.equals(mapped);
                }

                VNodeState s = vif.notifyNodeConnector(mgr, db, state, nc,
                                                       pstate, type);
                if (vif.isEnabled()) {
                    if (LOG.isTraceEnabled()) {
                        LOG.trace("{}:{}: notifyNodeConnector(if): {} -> {}",
                                  getContainerName(), bridgePath, state, s);
                    }
                    state = s;
                }
            }

            for (VlanMapImpl vmap: vlanMaps.values()) {
                if (!doflush && pstate != VNodeState.UP) {
                    VlanMapConfig vlconf = vmap.getVlanMapConfig();
                    Node vmnode = vlconf.getNode();
                    doflush = (vmnode == null || vmnode.equals(nc.getNode()));
                }

                VNodeState s = vmap.notifyNodeConnector(mgr, db, state, nc,
                                                        pstate, type);
                if (LOG.isTraceEnabled()) {
                    LOG.trace("{}:{}: notifyNodeConnector(vmap): {} -> {}",
                              getContainerName(), bridgePath, state, s);
                }
                state = s;
            }
            setState(mgr, state);

            if (doflush) {
                // Flush MAC address table entries associated with the given
                // node connector.
                MacAddressTable table = mgr.getMacAddressTable(bridgePath);
                table.flush(nc);
            }
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * This method is called when topology graph is changed.
     *
     * @param mgr       VTN Manager service.
     * @param topoList  List of topoedgeupdates Each topoedgeupdate includes
     *                  edge, its Properties (BandWidth and/or Latency etc)
     *                  and update type.
     */
    void edgeUpdate(VTNManagerImpl mgr, List<TopoEdgeUpdate> topoList) {
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();

        Lock wrlock = rwLock.readLock();
        VNodeState state = VNodeState.UNKNOWN;
        wrlock.lock();
        try {
            for (VBridgeIfImpl vif: vInterfaces.values()) {
                VNodeState s = vif.edgeUpdate(mgr, db, state, topoList);
                if (vif.isEnabled()) {
                    if (LOG.isTraceEnabled()) {
                        LOG.trace("{}:{}: edgeUpdate(if): {} -> {}",
                                  getContainerName(), bridgePath, state, s);
                    }
                    state = s;
                }
            }

            for (VlanMapImpl vmap: vlanMaps.values()) {
                VNodeState s = vmap.edgeUpdate(mgr, db, state, topoList);
                if (LOG.isTraceEnabled()) {
                    LOG.trace("{}:{}: edgeUpdate(vmap): {} -> {}",
                              getContainerName(), bridgePath, state, s);
                }
                state = s;
            }
            setState(mgr, state);
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Notify the listener of current configuration.
     *
     * @param mgr       VTN Manager service.
     * @param listener  VTN manager listener service.
     */
    void notifyConfiguration(VTNManagerImpl mgr, IVTNManagerAware listener) {
        UpdateType type = UpdateType.ADDED;
        VBridge vbridge = getVBridge(mgr);
        mgr.notifyChange(listener, bridgePath, vbridge, type);

        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            for (VBridgeIfImpl vif: vInterfaces.values()) {
                vif.notifyConfiguration(mgr, listener);
            }

            for (Map.Entry<String, VlanMapImpl> entry: vlanMaps.entrySet()) {
                String id = entry.getKey();
                VlanMapImpl vmap = entry.getValue();
                VlanMapConfig vlconf = vmap.getVlanMapConfig();
                VlanMap vlmap = new VlanMap(id, vlconf.getNode(),
                                            vlconf.getVlan());
                mgr.notifyChange(listener, bridgePath, vlmap, type);
            }
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Initiate the discovery of a host base on its IP address.
     *
     * @param mgr   VTN manager service.
     * @param pctx  The context of the ARP packet to send.
     */
    void findHost(VTNManagerImpl mgr, PacketContext pctx) {
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            // Flood the specified ARP request.
            flood(mgr, pctx);
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Send a unicast ARP request to the specified host.
     *
     * @param mgr   VTN manager service.
     * @param type  Mapping type to be tested.
     * @param pctx  The context of the ARP packet to send.
     * @return  A {@code Boolean} object is returned if the specified host
     *          belongs to this bridge. If a ARP request was actually sent to
     *          the network, {@code Boolean.TRUE} is returned.
     *          {@code null} is returned if the specified host does not
     *          belong to this bridge.
     */
    Boolean probeHost(VTNManagerImpl mgr, MapType type, PacketContext pctx) {
        NodeConnector nc = pctx.getOutgoingNodeConnector();
        assert nc != null;
        short vlan = pctx.getVlan();

        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            VBridgeNode bnode = match(mgr, type, nc, vlan);
            if (bnode == null) {
                return null;
            }

            if (!bnode.isEnabled()) {
                if (LOG.isDebugEnabled()) {
                    LOG.debug("{}:{}: " +
                              "Don't send ARP request to disabled network: {}",
                              getContainerName(), bnode.getPath(),
                              pctx.getDescription(nc));
                }
                return Boolean.FALSE;
            }

            if (LOG.isTraceEnabled()) {
                LOG.trace("{}:{}: Send ARP request for probing: {}",
                          getContainerName(), bridgePath,
                          pctx.getDescription(nc));
            }
            mgr.transmit(nc, pctx.getFrame());
        } finally {
            rdlock.unlock();
        }

        return Boolean.TRUE;
    }

    /**
     * Handler for receiving the packet.
     *
     * @param mgr   VTN manager service.
     * @param type  Mapping type to be tested.
     * @param pctx  The context of the received packet.
     * @return  A {@code PacketResult} which indicates the result of handler.
     */
    PacketResult receive(VTNManagerImpl mgr, MapType type, PacketContext pctx) {
        NodeConnector incoming = pctx.getIncomingNodeConnector();
        short vlan = pctx.getVlan();

        // Writer lock is required because this method may change the state
        // of the bridge.
        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            VBridgeNode bnode = match(mgr, type, incoming, vlan);
            if (bnode == null) {
                return PacketResult.IGNORED;
            }

            if (bnode.isEnabled()) {
                pctx.addNodePath(bridgePath);
                pctx.addNodePath(bnode.getPath());
                handlePacket(mgr, pctx);
            } else if (LOG.isDebugEnabled()) {
                LOG.debug("{}:{}: " +
                          "Ignore packet received from disabled network: {}",
                          getContainerName(), bnode.getPath(),
                          pctx.getDescription(incoming));
            }
        } finally {
            wrlock.unlock();
        }

        return PacketResult.KEEP_PROCESSING;
    }

    /**
     * Invoked when the recalculation of the all shortest path tree is done.
     *
     * @param mgr  VTN manager service.
     */
    void recalculateDone(VTNManagerImpl mgr) {
        // Writer lock is required because this method may change the state
        // of the bridge.
        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
            VBridgeState bst = getBridgeState(db);
            if (bst.getFaultedPathSize() == 0) {
                return;
            }

            // Remove resolved node paths from the set of faulted node paths.
            List<ObjectPair<Node, Node>> resolved =
                bst.removeResolvedPath(mgr.getRouting());
            if (LOG.isInfoEnabled()) {
                for (ObjectPair<Node, Node> npath: resolved) {
                    LOG.info("{}:{}: Path fault resolved: {} -> {}",
                             getContainerName(), bridgePath, npath.getLeft(),
                             npath.getRight());
                }
            }

            if (bst.getFaultedPathSize() == 0) {
                updateState(mgr, db, bst);
            } else {
                setState(mgr, db, bst, VNodeState.DOWN);
            }
        } finally {
            wrlock.unlock();
        }
    }

    /**
     * Destroy the virtual L2 bridge.
     *
     * @param mgr     VTN manager service.
     * @param retain  {@code true} means that the parent tenant will be
     *                retained. {@code false} means that the parent tenant
     *                is being destroyed.
     */
    void destroy(VTNManagerImpl mgr, boolean retain) {
        VBridge vbridge = getVBridge(mgr);
        IVTNResourceManager resMgr = mgr.getResourceManager();

        Lock wrlock = rwLock.writeLock();
        wrlock.lock();
        try {
            // Destroy all VLAN mappings.
            for (Iterator<Map.Entry<String, VlanMapImpl>> it =
                     vlanMaps.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, VlanMapImpl> entry = it.next();
                String mapId = entry.getKey();
                VlanMapImpl vmap = entry.getValue();
                VlanMapConfig vlconf = vmap.getVlanMapConfig();
                short vlan = vlconf.getVlan();
                Short vid = Short.valueOf(vlan);
                if (vlanIdIndex.remove(vid) != null) {
                    resMgr.unregisterVlanMap(vlan);
                }

                VlanMap vlmap = new VlanMap(mapId, vlconf.getNode(), vlan);
                vmap.destroy(mgr, bridgePath, vlmap, false);
                it.remove();
            }

            // Destroy MAC address table.
            mgr.removeMacAddressTable(bridgePath, retain);

            // Destroy all interfaces.
            for (Iterator<VBridgeIfImpl> it = vInterfaces.values().iterator();
                 it.hasNext();) {
                VBridgeIfImpl vif = it.next();
                vif.destroy(mgr, false);
                it.remove();
            }

            if (retain) {
                // Purge all VTN flows related to this bridge.
                VTNThreadData.removeFlows(mgr, bridgePath);
            }

            // Unlink parent for GC.
            parent = null;
        } finally {
            wrlock.unlock();
        }

        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
        db.remove(bridgePath);
        VBridgeEvent.removed(mgr, bridgePath, vbridge, retain);
    }

    /**
     * Return a runtime state object for the virtual bridge.
     *
     * @param mgr  VTN Manager service.
     * @return  A runtume state object.
     */
    private VBridgeState getBridgeState(VTNManagerImpl mgr) {
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
        return getBridgeState(db);
    }

    /**
     * Return a runtime state object for the virtual bridge.
     *
     * @param db  Runtime state DB.
     * @return  A runtume state object.
     */
    private VBridgeState getBridgeState(ConcurrentMap<VTenantPath, Object> db) {
        VBridgeState bst = (VBridgeState)db.get(bridgePath);
        if (bst == null) {
            bst = new VBridgeState(VNodeState.UNKNOWN);
        }

        return bst;
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
        if (!(o instanceof VBridgeImpl)) {
            return false;
        }

        VBridgeImpl vbr = (VBridgeImpl)o;
        if (!bridgePath.equals(vbr.bridgePath)) {
            return false;
        }

        VBridgeConfig bconf = getVBridgeConfig();
        VBridgeConfig otherBconf = vbr.getVBridgeConfig();
        if (!bconf.equals(otherBconf)) {
            return false;
        }

        // Compare copied maps in order to avoid deadlock.
        Map<String, VBridgeIfImpl> ifs = getInterfaceMap();
        Map<String, VBridgeIfImpl> otherIfs = vbr.getInterfaceMap();
        if (!ifs.equals(otherIfs)) {
            return false;
        }

        Map<String, VlanMapImpl> vmaps = getVlanMappings();
        Map<String, VlanMapImpl> otherVmaps = vbr.getVlanMappings();

        return vmaps.equals(otherVmaps);
    }

    /**
     * Return the hash code of this object.
     *
     * @return  The hash code.
     */
    @Override
    public int hashCode() {
        int h = bridgePath.hashCode() ^ getVBridgeConfig().hashCode();

        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            h += vInterfaces.hashCode() ^ vlanMaps.hashCode();
        } finally {
            rdlock.unlock();
        }

        return h;
    }

    /**
     * Merge the given vBridge configuration to the current configuration.
     *
     * <p>
     *   If at least one field in {@code bconf} keeps a valid value, this
     *   method creates a shallow copy of the current configuration, and set
     *   valid values in {@code bconf} to the copy.
     * </p>
     *
     * @param bconf  Configuration to be merged.
     * @return  A merged {@code VBridgeConfig} object.
     */
    private synchronized VBridgeConfig merge(VBridgeConfig bconf) {
        String desc = bconf.getDescription();
        int age = bconf.getAgeInterval();
        if (desc == null) {
            return (age < 0)
                ? bridgeConfig
                : new VBridgeConfig(bridgeConfig.getDescription(), age);
        } else if (age < 0) {
            return new VBridgeConfig(desc, bridgeConfig.getAgeInterval());
        }

        return bconf;
    }

    /**
     * Resolve undefined attributes in the specified bridge configuration.
     *
     * @param bconf  The bridge configuration.
     * @return       {@code VBridgeConfig} to be applied.
     */
    private VBridgeConfig resolve(VBridgeConfig bconf) {
        int age = bconf.getAgeInterval();
        if (age < 0) {
            return new VBridgeConfig(bconf.getDescription(),
                                     DEFAULT_AGE_INTERVAL);
        }

        return bconf;
    }

    /**
     * Ensure that the specified bridge configuration is valid.
     *
     * @param bconf  The bridge configuration to be tested.
     * @throws VTNException  An error occurred.
     */
    private void checkConfig(VBridgeConfig bconf) throws VTNException {
        int ival = bconf.getAgeInterval();
        if (ival < MIN_AGE_INTERVAL || ival > MAX_AGE_INTERVAL) {
            throw new VTNException(StatusCode.BADREQUEST,
                                   "Invalid MAC address aging interval");
        }
    }

    /**
     * Return a shallow copy of the virtual interface map.
     *
     * @return  Pairs of interface name and interface instance.
     */
    private Map<String, VBridgeIfImpl> getInterfaceMap() {
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            return new TreeMap<String, VBridgeIfImpl>(vInterfaces);
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Return a shallow copy of the VLAN mappings.
     *
     * @return Pairs of VLAN mapping ID and VLAN mapping instance.
     */
    private Map<String, VlanMapImpl> getVlanMappings() {
        Lock rdlock = rwLock.readLock();
        rdlock.lock();
        try {
            return new TreeMap<String, VlanMapImpl>(vlanMaps);
        } finally {
            rdlock.unlock();
        }
    }

    /**
     * Read data from the given input stream and deserialize.
     *
     * @param in  An input stream.
     * @throws IOException
     *    An I/O error occurred.
     * @throws ClassNotFoundException
     *    At least one necessary class was not found.
     */
    @SuppressWarnings("unused")
    private void readObject(ObjectInputStream in)
        throws IOException, ClassNotFoundException {
        in.defaultReadObject();

        // Reset the lock.
        rwLock = new ReentrantReadWriteLock();

        // Rebuild VLAN mapping index to reduce memory footprint.
        vlanIdIndex = new TreeMap<Short, VlanIdMap>();
        for (VlanMapImpl vmap: vlanMaps.values()) {
            try {
                addVlanIdIndex(vmap);
            } catch (Exception e) {
                // This should never happen. */
                LOG.error("Unexpected exception", e);
            }
        }
    }

    /**
     * Add the given VLAN mapping to the VLAN ID index.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param vmap  VLAN mapping instance to be added.
     * @return  {@code true} is returned if no other VLAN mapping in this
     *          bridge does not map the same VLAN ID as the given VLAN mapping.
     *          {@code false} is returned if at least one VLAN mapping in this
     *          bridge maps the same VLAN ID as the given VLAN mapping.
     * @throws VTNException
     *     The VLAN associated with the given VLAN mapping is already mapped
     *     to this bridge.
     */
    private boolean addVlanIdIndex(VlanMapImpl vmap) throws VTNException {
        VlanMapConfig vlc = vmap.getVlanMapConfig();
        short vlan = vlc.getVlan();
        VlanIdMap idmap = vlanIdIndex.get(vlan);
        boolean ret;
        if (idmap == null) {
            idmap = VlanIdMap.create(vmap);
            vlanIdIndex.put(vlan, idmap);
            ret = true;
        } else {
            assert !idmap.isEmpty();
            idmap.add(vmap);
            ret = false;
        }

        return ret;
    }

    /**
     * Remove the given VLAN mapping from the VLAN ID index.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param vlconf  VLAN mapping configuration.
     * @return  {@code true} is returned if the VLAN ID associated with the
     *          given VLAN mapping needs to be unregistered from the global
     *          resource manager. Otherwise {@code false} is returned.
     */
    private boolean removeVlanIdIndex(VlanMapConfig vlconf) {
        short vlan = vlconf.getVlan();
        Node node = vlconf.getNode();
        VlanIdMap idmap = vlanIdIndex.get(vlan);
        boolean ret = idmap.remove(node);
        if (ret) {
            vlanIdIndex.remove(vlan);
        }

        return ret;
    }

    /**
     * Return a failure status that indicates the specified interface does not
     * exist.
     *
     * @param ifName  The name of the interface.
     * @return  A failure status.
     */
    private Status interfaceNotFound(String ifName) {
        String msg = ifName + ": Interface does not exist";
        return new Status(StatusCode.NOTFOUND, msg);
    }

    /**
     * Return a failure status that indicates the specified VLAN mapping does
     * not exist.
     *
     * @param id      The identifier of the VLAN mapping.
     * @return  A failure status.
     */
    private Status vlanMapNotFound(String id) {
        String msg = id + ": VLAN mapping does not exist";
        return new Status(StatusCode.NOTFOUND, msg);
    }

    /**
     * Return the virtual interface instance associated with the given name.
     *
     * <p>
     *   This method must be called with the bridge lock.
     * </p>
     *
     * @param path  Path to the interface.
     * @return  Virtual interface instance is returned.
     * @throws VTNException  An error occurred.
     */
    private VBridgeIfImpl getInterfaceImpl(VBridgeIfPath path)
        throws VTNException {
        String ifName = path.getInterfaceName();
        if (ifName == null) {
            Status status = VTNManagerImpl.argumentIsNull("Interface name");
            throw new VTNException(status);
        }

        VBridgeIfImpl vif = vInterfaces.get(ifName);
        if (vif == null) {
            Status status = interfaceNotFound(ifName);
            throw new VTNException(status);
        }

        return vif;
    }

    /**
     * Set state of the bridge.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param mgr    VTN Manager service.
     * @param state  New bridge state.
     */
    private void setState(VTNManagerImpl mgr, VNodeState state) {
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
        VBridgeState bst = getBridgeState(db);
        setState(mgr, db, bst, state);
    }

    /**
     * Set state of the bridge.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param mgr    VTN Manager service.
     * @param db     Virtual node state DB.
     * @param bst    Runtime state of the bridge.
     * @param state  New bridge state.
     */
    private void setState(VTNManagerImpl mgr,
                          ConcurrentMap<VTenantPath, Object> db,
                          VBridgeState bst, VNodeState state) {
        VNodeState st = bst.setState(state);
        if (bst.isDirty()) {
            db.put(bridgePath, bst);
            int faulted = bst.getFaultedPathSize();
            VBridge vbridge = new VBridge(getName(), st, faulted,
                                          getVBridgeConfig());
            VBridgeEvent.changed(mgr, bridgePath, vbridge, false);
        }
    }

    /**
     * Add a node path to the set of faulted node paths.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param mgr    VTN Manager service.
     * @param snode  The source node.
     * @param dnode  The destination node.
     */
    private void addFaultedPath(VTNManagerImpl mgr, Node snode, Node dnode) {
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
        VBridgeState bst = getBridgeState(db);
        bst.addFaultedPath(snode, dnode);
        setState(mgr, db, bst, VNodeState.DOWN);
    }

    /**
     * Scan interfaces and VLAN mappings, and determine current state of the
     * virtual bridge.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param mgr   VTN Manager service.
     */
    private void updateState(VTNManagerImpl mgr) {
        ConcurrentMap<VTenantPath, Object> db = mgr.getStateDB();
        VBridgeState bst = getBridgeState(db);
        updateState(mgr, db, bst);
    }

    /**
     * Scan interfaces and VLAN mappings, and determine current state of the
     * virtual bridge.
     *
     * <p>
     *   Note that this method must be called with holding the bridge write
     *   lock.
     * </p>
     *
     * @param mgr   VTN Manager service.
     * @param db  Runtime state DB.
     * @param bst    Runtime state of the bridge.
     */
    private void updateState(VTNManagerImpl mgr,
                             ConcurrentMap<VTenantPath, Object> db,
                             VBridgeState bst) {
        VNodeState state = VNodeState.UNKNOWN;

        // Scan virtual interfaces.
        for (VBridgeIfImpl vif: vInterfaces.values()) {
            if (vif.isEnabled()) {
                state = vif.getBridgeState(db, state);
                if (state == VNodeState.DOWN) {
                    setState(mgr, db, bst, state);
                    return;
                }
            }
        }

        // Scan VLAN mappings.
        for (VlanMapImpl vmap: vlanMaps.values()) {
            state = vmap.getBridgeState(db, state);
            if (state == VNodeState.DOWN) {
                break;
            }
        }

        setState(mgr, db, bst, state);
    }

    /**
     * Return information about the bridge.
     *
     * @param mgr    VTN Manager service.
     * @param name   The name of the bridge.
     * @param bconf  Bridge configuration.
     * @return  Information about the bridge.
     */
    private VBridge getVBridge(VTNManagerImpl mgr, String name,
                               VBridgeConfig bconf) {
        VBridgeState bst = getBridgeState(mgr);
        int faulted = bst.getFaultedPathSize();
        return new VBridge(name, bst.getState(), faulted, bconf);
    }

    /**
     * Handle the received packet.
     *
     * <p>
     *   Note that this method must be called with holding the bridge lock.
     * </p>
     *
     * @param mgr   VTN Manager service.
     * @param pctx  The context of the received packet.
     */
    private void handlePacket(VTNManagerImpl mgr, PacketContext pctx) {
        // Learn the source MAC address if needed.
        MacAddressTable table = mgr.getMacAddressTable(bridgePath);
        table.add(pctx);

        byte[] dst = pctx.getDestinationAddress();
        if (isResponseToController(mgr, pctx, dst)) {
            return;
        }

        // Determine whether the destination address is known or not.
        MacTableEntry tent = getDestination(mgr, pctx, table, dst);
        if (tent == null) {
            return;
        }

        // Ensure that the destination host is reachable.
        NodeConnector incoming = pctx.getIncomingNodeConnector();
        NodeConnector outgoing = tent.getPort();
        Node snode = incoming.getNode();
        Node dnode = outgoing.getNode();
        Path path;
        if (!snode.equals(dnode)) {
            IRouting routing = mgr.getRouting();
            path = routing.getRoute(snode, dnode);
            if (path == null) {
                LOG.error("{}:{}: Path fault: {} -> {}",
                          getContainerName(), bridgePath, snode, dnode);
                addFaultedPath(mgr, snode, dnode);
                return;
            }
        } else {
            path = null;
        }

        // Forward the packet.
        short outVlan = tent.getVlan();
        Ethernet frame = pctx.createFrame(outVlan);
        if (LOG.isTraceEnabled()) {
            LOG.trace("{}:{}: Forward packet to known host: {}",
                      getContainerName(), bridgePath,
                      pctx.getDescription(frame, outgoing, outVlan));
        }
        mgr.transmit(outgoing, frame);

        // Install VTN flow.
        installFlow(mgr, pctx, tent, path);
    }

    /**
     * Determine whether the destination address of the received packet is
     * this controller or not.
     *
     * @param mgr   VTN Manager service.
     * @param pctx  The context of the received packet.
     * @param dst   The destination MAC address of the received packet.
     * @return  {@code true} is returned if the given destination MAC address
     *          is same as the MAC address of this controller.
     *          Otherwise {@code false} is returned.
     */
    private boolean isResponseToController(VTNManagerImpl mgr,
                                           PacketContext pctx, byte[] dst) {
        ISwitchManager swMgr = mgr.getSwitchManager();
        byte[] ctlrMac = swMgr.getControllerMAC();
        if (Arrays.equals(ctlrMac, dst)) {
            if (LOG.isTraceEnabled()) {
                NodeConnector incoming = pctx.getIncomingNodeConnector();
                LOG.trace("{}:{}: Ignore packet sent to controller: {}",
                          getContainerName(), bridgePath,
                          pctx.getDescription(incoming));
            }
            return true;
        }
        return false;
    }

    /**
     * Return the MAC address table entry associated with the destination
     * address of the received packet.
     *
     * <p>
     *   Note that this method may flood the received packet to this bridge
     *   if needed.
     * </p>
     *
     * @param mgr    VTN Manager service.
     * @param pctx   The context of the received packet.
     * @param table  The MAC address table for this bridge.
     * @param dst    The destination MAC address of the received packet.
     * @return  A {@link MacTableEntry} object is returned if found.
     *          {@code null} is returned if not found or if the received packet
     *          should not be sent to this bridge.
     */
    private MacTableEntry getDestination(VTNManagerImpl mgr,
                                         PacketContext pctx,
                                         MacAddressTable table, byte[] dst) {
        if (!NetUtils.isUnicastMACAddr(dst)) {
            // Flood the non-unicast packet.
            flood(mgr, pctx);
            return null;
        }

        Long key = MacAddressTable.getTableKey(dst);
        MacTableEntry tent = table.get(key);
        if (tent == null) {
            // Flood the received packet.
            flood(mgr, pctx);
            return null;
        }

        // Ensure that the outgoing network is mapped to this bridge.
        NodeConnector outgoing = tent.getPort();
        short outVlan = tent.getVlan();
        VBridgeNode bnode = match(mgr, MapType.ALL, outgoing, outVlan);
        if (bnode == null) {
            LOG.warn("{}:{}: Unexpected MAC address entry: {}",
                     getContainerName(), bridgePath, tent);
            pctx.addObsoleteEntry(key, tent);
            table.remove(key);
            flood(mgr, pctx);
            return null;
        }

        if (!bnode.isEnabled()) {
            if (LOG.isDebugEnabled()) {
                LOG.debug("{}:{}: Drop packet due to disabled network: {}",
                          getContainerName(), bnode.getPath(),
                          pctx.getDescription(outgoing));
            }
            return null;
        }

        if (!mgr.isEnabled(outgoing)) {
            if (LOG.isWarnEnabled()) {
                LOG.warn("{}:{}: Drop packet because outgoing port is down: " +
                         "{}", getContainerName(), bridgePath,
                         pctx.getDescription(outgoing));
            }
            return null;
        }

        pctx.addNodePath(bnode.getPath());

        return tent;
    }

    /**
     * Install a VTN flow for the received packet.
     *
     * @param mgr   VTN Manager service.
     * @param pctx  The context of the received packet.
     * @param tent  The MAC address table entry associated with the destination
     *              address of the received packet.
     * @param path  Path to the destination address of the received packet.
     */
    private void installFlow(VTNManagerImpl mgr, PacketContext pctx,
                             MacTableEntry tent, Path path) {
        // Create flow entries.
        VTNFlowDatabase fdb = mgr.getTenantFlowDB(getTenantName());
        if (fdb == null) {
            // This should never happen.
            LOG.warn("{}:{}: No flow database",
                     getContainerName(), bridgePath);
            return;
        }

        // Purge obsolete flows before installing new flow.
        pctx.purgeObsoleteFlow(mgr, fdb);

        NodeConnector incoming = pctx.getIncomingNodeConnector();
        VTNConfig vc = mgr.getVTNConfig();
        int pri = vc.getL2FlowPriority();
        VTNFlow vflow = fdb.create(mgr);
        Match match;
        if (path != null) {
            // Create flow entries except for egress flow.
            for (Edge edge: path.getEdges()) {
                match = pctx.createMatch(incoming);
                NodeConnector port = edge.getTailNodeConnector();
                ActionList actions = new ActionList(port.getNode());
                actions.addOutput(port);
                vflow.addFlow(mgr, match, actions, pri);
                incoming = edge.getHeadNodeConnector();
            }
        }

        // Create egress flow entry.
        NodeConnector outgoing = tent.getPort();
        short outVlan = tent.getVlan();
        Node dnode = outgoing.getNode();
        assert incoming.getNode().equals(outgoing.getNode());
        match = pctx.createMatch(incoming);
        ActionList actions = new ActionList(dnode);
        actions.addVlanId(outVlan).addOutput(outgoing);
        vflow.addFlow(mgr, match, actions, pri);

        // Set flow timeout.
        VTenantConfig tconf = parent.getVTenantConfig();
        vflow.setTimeout(tconf.getIdleTimeout(), tconf.getHardTimeout());

        // Set flow dependency which specifies network elements relevant to
        // this VTN flow.
        pctx.setFlowDependency(vflow);
        byte[] dst = pctx.getDestinationAddress();
        vflow.addDependency(new MacVlan(dst, outVlan));

        // Install flow entries.
        fdb.install(mgr, vflow);
    }

    /**
     * Determine whether the network specified by the switch port and the
     * VLAN ID is mapped to this bridge or not.
     *
     * <p>
     *   Note that this method must be called with holding the bridge lock.
     * </p>
     *
     * @param mgr   VTN Manager service.
     * @param type  Mapping type to be tested.
     * @param nc    A node connector where the packet was received.
     * @param vlan  VLAN ID in the received packet.
     * @return  A {@code VBridgeNode} is returned if the network specified
     *          by {@code nc} and {@code vlan} is mapped to this bridge.
     *          Otherwise {@code null} is returned.
     */
    private VBridgeNode match(VTNManagerImpl mgr, MapType type,
                              NodeConnector nc, short vlan) {
        if (type.match(MapType.PORT)) {
            // Check whether the packet is mapped by port mappings or not.
            for (VBridgeIfImpl vif: vInterfaces.values()) {
                if (vif.match(mgr, nc, vlan)) {
                    return vif;
                }
            }
        }

        if (type.match(MapType.VLAN)) {
            // Check whether the packet is mapped by VLAN mappings or not.
            VlanIdMap idmap = vlanIdIndex.get(vlan);
            if (idmap != null) {
                Node node = nc.getNode();
                VlanMapImpl vmap = idmap.match(node);
                if (vmap != null) {
                    return vmap;
                }
            }
        }

        return null;
    }

    /**
     * Flood the specified packet to this bridge.
     *
     * <p>
     *   Note that this method must be called with holding the bridge lock.
     * </p>
     *
     * @param mgr   VTN manager service.
     * @param pctx  The context of the packet.
     */
    private void flood(VTNManagerImpl mgr, PacketContext pctx) {
        // Don't send the packet to the incoming network.
        HashSet<PortVlan> sent = new HashSet<PortVlan>();
        PortVlan innw = pctx.getIncomingNetwork();
        if (innw != null) {
            sent.add(innw);
        }

        // Forward packet to the network established by the port mapping.
        for (VBridgeIfImpl vif: vInterfaces.values()) {
            vif.transmit(mgr, pctx, sent);
        }

        // Forward packet to the network established by the VLAN mapping.
        for (VlanMapImpl vmap: vlanMaps.values()) {
            vmap.transmit(mgr, pctx, sent);
        }
    }
}
