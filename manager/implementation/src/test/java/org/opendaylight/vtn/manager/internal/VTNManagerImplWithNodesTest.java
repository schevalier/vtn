/*
 * Copyright (c) 2013 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.manager.internal;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.apache.felix.dm.impl.ComponentImpl;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.opendaylight.controller.forwardingrulesmanager.FlowEntry;
import org.opendaylight.controller.hosttracker.hostAware.HostNodeConnector;
import org.opendaylight.controller.sal.core.ConstructionException;
import org.opendaylight.controller.sal.core.Edge;
import org.opendaylight.controller.sal.core.Node;
import org.opendaylight.controller.sal.core.NodeConnector;
import org.opendaylight.controller.sal.core.Property;
import org.opendaylight.controller.sal.core.UpdateType;
import org.opendaylight.controller.sal.packet.ARP;
import org.opendaylight.controller.sal.packet.Ethernet;
import org.opendaylight.controller.sal.packet.IEEE8021Q;
import org.opendaylight.controller.sal.packet.Packet;
import org.opendaylight.controller.sal.packet.PacketResult;
import org.opendaylight.controller.sal.packet.RawPacket;
import org.opendaylight.controller.sal.packet.address.EthernetAddress;
import org.opendaylight.controller.sal.topology.TopoEdgeUpdate;
import org.opendaylight.controller.sal.utils.EtherTypes;
import org.opendaylight.controller.sal.utils.NodeConnectorCreator;
import org.opendaylight.controller.sal.utils.NodeCreator;
import org.opendaylight.controller.sal.utils.Status;
import org.opendaylight.controller.sal.utils.StatusCode;
import org.opendaylight.controller.switchmanager.ISwitchManager;
import org.opendaylight.controller.topologymanager.ITopologyManager;
import org.opendaylight.vtn.manager.MacAddressEntry;
import org.opendaylight.vtn.manager.PortMap;
import org.opendaylight.vtn.manager.PortMapConfig;
import org.opendaylight.vtn.manager.SwitchPort;
import org.opendaylight.vtn.manager.VBridge;
import org.opendaylight.vtn.manager.VBridgeConfig;
import org.opendaylight.vtn.manager.VBridgeIfPath;
import org.opendaylight.vtn.manager.VBridgePath;
import org.opendaylight.vtn.manager.VInterface;
import org.opendaylight.vtn.manager.VInterfaceConfig;
import org.opendaylight.vtn.manager.VNodeState;
import org.opendaylight.vtn.manager.VTNException;
import org.opendaylight.vtn.manager.VTenantPath;
import org.opendaylight.vtn.manager.VlanMap;
import org.opendaylight.vtn.manager.VlanMapConfig;
import org.opendaylight.vtn.manager.internal.cluster.FlowModResult;
import org.opendaylight.vtn.manager.internal.cluster.MacTableEntry;
import org.opendaylight.vtn.manager.internal.cluster.PortVlan;

/**
 * JUnit test for {@link VTNManagerImplTest}.
 * This test class test at using stub environment.
 */
public class VTNManagerImplWithNodesTest extends VTNManagerImplTestCommon {

    @BeforeClass
    public static void beforeClass() {
        stubMode = 2;
    }

    @Test
    public void testInitDestory() {
        ComponentImpl c = new ComponentImpl(null, null, null);
        Hashtable<String, String> properties = new Hashtable<String, String>();
        String containerName = "default";
        properties.put("containerName", containerName);
        c.setServiceProperties(properties);

        Map<VBridgeIfPath, PortMapConfig> pmaps = new HashMap<VBridgeIfPath, PortMapConfig>();
        Map<VlanMap, VlanMapConfig> vmaps = new HashMap<VlanMap, VlanMapConfig>();
        Map<VBridgePath, VNodeState> brstates = new HashMap<VBridgePath, VNodeState>();
        Map<VBridgeIfPath, VNodeState> ifstates = new HashMap<VBridgeIfPath, VNodeState>();

        // add VTN config
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        VBridgeIfPath ifpath1 = new VBridgeIfPath(tname, bname, "vif1");
        VBridgeIfPath ifpath2 = new VBridgeIfPath(tname, bname, "vif2");
        VBridgeIfPath ifpath3 = new VBridgeIfPath(tname, bname, "vif3");

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifpathlist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifpathlist.add(ifpath1);
        ifpathlist.add(ifpath3);
        createTenantAndBridgeAndInterface(vtnMgr, tpath, bpathlist, ifpathlist);

        Status st = vtnMgr.addBridgeInterface(ifpath2, new VInterfaceConfig(null, false));
        assertTrue(st.toString(), st.isSuccess());
        ifpathlist.add(ifpath2);

        // restart VTNManager and check config after restart.
        restartVTNManager(c);

        pmaps.put(ifpath1, null);
        pmaps.put(ifpath2, null);
        pmaps.put(ifpath3, null);
        checkVTNconfig(tpath, bpathlist, pmaps, null);

        // add mappings
        Node node = NodeCreator.createOFNode(0L);
        SwitchPort port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW,
        String.valueOf(10));
        PortMapConfig pmconf = new PortMapConfig(node, port, (short)0);
        st = vtnMgr.setPortMap(ifpath1, pmconf);
        assertTrue(st.toString(), st.isSuccess());
        pmaps.put(ifpath1, pmconf);
        pmaps.put(ifpath2, null);

        VlanMapConfig vlconf = new VlanMapConfig(null, (short)1);
        VlanMap map = null;
        try {
            map = vtnMgr.addVlanMap(bpath, vlconf);
        } catch (VTNException e) {
            unexpected(e);
        }
        vmaps.put(map, vlconf);

        vlconf = new VlanMapConfig(null, (short)4095);
        map = null;
        try {
            map = vtnMgr.addVlanMap(bpath, vlconf);
        } catch (VTNException e) {
            unexpected(e);
        }
        vmaps.put(map, vlconf);

        checkVTNconfig(tpath, bpathlist, pmaps, vmaps);

        brstates.put(bpath, VNodeState.UP);
        ifstates.put(ifpath1, VNodeState.UP);
        ifstates.put(ifpath2, VNodeState.DOWN);
        ifstates.put(ifpath3, VNodeState.UNKNOWN);
        checkNodeState(tpath, brstates, ifstates);

        // restart VTNManager and check config after restart.
        restartVTNManager(c);

        checkVTNconfig(tpath, bpathlist, pmaps, vmaps);
        checkNodeState(tpath, brstates, ifstates);

        // add vlanmap with not existing node.
        node = NodeCreator.createOFNode(Long.valueOf((long)100));
        vlconf = new VlanMapConfig(node, (short)100);
        map = null;
        try {
            map = vtnMgr.addVlanMap(bpath, vlconf);
        } catch (VTNException e) {
            unexpected(e);
        }
        vmaps.put(map, vlconf);
        checkVTNconfig(tpath, bpathlist, pmaps, vmaps);

        brstates.put(bpath, VNodeState.DOWN);
        checkNodeState(tpath, brstates, ifstates);

        restartVTNManager(c);

        // this case, startup with cache.
        checkVTNconfig(tpath, bpathlist, pmaps, vmaps);
        checkNodeState(tpath, brstates, ifstates);

        stopVTNManager(true);
        resMgr.cleanUp("default");
        startVTNManager(c);

        // this case, startup with no cache.
        checkVTNconfig(tpath, bpathlist, pmaps, vmaps);
        checkNodeState(tpath, brstates, ifstates);

        stopVTNManager(true);
    }

    @Test
    public void testInitDestoryAfterInvalidMap() {
        ComponentImpl c = new ComponentImpl(null, null, null);
        Hashtable<String, String> properties = new Hashtable<String, String>();
        String containerName = "default";
        properties.put("containerName", containerName);
        c.setServiceProperties(properties);

        Map<VBridgeIfPath, PortMapConfig> pmaps = new HashMap<VBridgeIfPath, PortMapConfig>();
        Map<VlanMap, VlanMapConfig> vmaps = new HashMap<VlanMap, VlanMapConfig>();
        Map<VBridgePath, VNodeState> brstates = new HashMap<VBridgePath, VNodeState>();
        Map<VBridgeIfPath, VNodeState> ifstates = new HashMap<VBridgeIfPath, VNodeState>();

        stopVTNManager(false);
        TestStub stub = new TestStub(0);
        vtnMgr.setSwitchManager(stub);
        vtnMgr.setTopologyManager(stub);
        startVTNManager(c);

        // add VTN config
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        VBridgeIfPath ifpath1 = new VBridgeIfPath(tname, bname, "vif1");
        VBridgeIfPath ifpath2 = new VBridgeIfPath(tname, bname, "vif2");

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifpathlist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifpathlist.add(ifpath1);
        ifpathlist.add(ifpath2);
        createTenantAndBridgeAndInterface(vtnMgr, tpath, bpathlist, ifpathlist);

        Node node = NodeCreator.createOFNode(0L);
        SwitchPort port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW,
        String.valueOf(10));
        PortMapConfig pmconf = new PortMapConfig(node, port, (short)0);
        Status st = vtnMgr.setPortMap(ifpath1, pmconf);
        assertTrue(st.toString(), st.isSuccess());

        st = vtnMgr.setPortMap(ifpath2, pmconf);
        assertTrue(st.toString(), st.isSuccess());

        stopVTNManager(false);
        vtnMgr.setSwitchManager(stubObj);
        vtnMgr.setTopologyManager(stubObj);
        startVTNManager(c);

        brstates.put(bpath, VNodeState.DOWN);
        ifstates.put(ifpath1, VNodeState.UP);
        ifstates.put(ifpath2, VNodeState.DOWN);
        checkNodeState(tpath, brstates, ifstates);
    }

    /**
     * check VBridge and VInterface state, used in testInitDestory().
     * @param tpath
     * @param bpaths
     * @param ifpaths
     */
    private void checkNodeState(VTenantPath tpath, Map<VBridgePath, VNodeState> bpaths,
            Map<VBridgeIfPath, VNodeState> ifpaths) {
        List<VBridge> blist = null;
        List<VInterface> iflist = null;

        if (bpaths != null) {
            // check vbridge state.
            try {
                blist = vtnMgr.getBridges(tpath);
            } catch (VTNException e) {
                unexpected(e);
            }

            VBridge vbr = blist.get(0);
            for (Map.Entry<VBridgePath, VNodeState> ent : bpaths.entrySet()) {
                assertEquals(ent.getKey().toString(), ent.getKey().getBridgeName(), vbr.getName());
                assertEquals(ent.getKey().toString(), ent.getValue(), vbr.getState());
            }
        }

        if (ifpaths != null) {
            // check vinterface state.
            for (VBridgePath bpath : bpaths.keySet()) {
                try {
                    iflist = vtnMgr.getBridgeInterfaces(bpath);
                } catch (VTNException e) {
                    unexpected(e);
                }

                assertEquals(ifpaths.size(), iflist.size());
                for (VInterface vif : iflist) {
                    for (Map.Entry<VBridgeIfPath, VNodeState> ent : ifpaths.entrySet()) {
                        if (vif.getName().equals(ent.getKey().getInterfaceName())) {
                            assertEquals(ent.getValue(), vif.getState());
                        }
                    }
                }
            }
        }
    }

    /**
     * Test method for
     * {@link VTNManagerImpl#addVlanMap(VBridgePath, VlanMapConfig)},
     * {@link VTNManagerImpl#removeVlanMap(VBridgePath, java.lang.String)},
     * {@link VTNManagerImpl#getVlanMap(VBridgePath, java.lang.String)},
     * {@link VTNManagerImpl#getVlanMaps(VBridgePath)}.
     */
    @Test
    public void testVlanMap() {
        VTNManagerImpl mgr = vtnMgr;
        short[] vlans = new short[] { 0, 10, 4095 };
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        String ifname = "vinterface";
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname, ifname);
        Status st = null;
        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp);

        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        // add a vlanmap to a vbridge
        testVlanMapCheck(mgr, bpath);

        // add multi vlanmap to a vbridge
        for (Node node : createNodes(3)) {
            for (short vlan : vlans) {
                VlanMapConfig vlconf = new VlanMapConfig(node, vlan);
                VlanMap map = null;
                try {
                    map = mgr.addVlanMap(bpath, vlconf);
                    if (node != null && node.getType() != NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        fail("throwing Exception was expected.");
                    }
                } catch (VTNException e) {
                    if (node != null && node.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        unexpected(e);
                    } else {
                        // if node type is not OF, throwing a VTNException is expected.
                        continue;
                    }
                }
            }

            List<VlanMap> list = null;
            try {
                list = mgr.getVlanMaps(bpath);
            } catch (Exception e) {
                unexpected(e);
            }
            if (node == null || node.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                assertTrue((node == null) ? "" : node.toString(), list.size() == vlans.length);
                VBridge brdg = null;
                try {
                    brdg = mgr.getBridge(bpath);
                } catch (VTNException e) {
                   unexpected(e);
                }
                assertEquals((node == null) ? "" : node.toString(), VNodeState.UP, brdg.getState());
            } else {
                assertTrue(list.size() == 0);
                VBridge brdg = null;
                try {
                    brdg = mgr.getBridge(bpath);
                } catch (VTNException e) {
                   unexpected(e);
                }
                assertEquals(node.toString(), VNodeState.UNKNOWN, brdg.getState());
            }

            for (VlanMap map : list) {
                st = mgr.removeVlanMap(bpath, map.getId());
                assertTrue((node == null) ? "" : node.toString(), st.isSuccess());
            }
        }

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * Test method for
     * {@link VTNManagerImpl#getPortMap(VBridgeIfPath)} and
     * {@link VTNManagerImpl#setPortMap(VBridgeIfPath, PortMapConfig)}.
     */
    @Test
    public void testPortMap() {
        VTNManagerImpl mgr = vtnMgr;
        short[] vlans = new short[] { 0, 10, 4095 };
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname, "vinterface");
        Status st = null;
        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp);

        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        PortMap pmap = null;
        try {
            pmap = mgr.getPortMap(ifp);
        } catch (Exception e) {
            unexpected(e);
        }
        assertNull(pmap);

        Node node = NodeCreator.createOFNode(0L);
        SwitchPort[] ports = new SwitchPort[] {
                new SwitchPort("port-10", NodeConnector.NodeConnectorIDType.OPENFLOW, "10"),
                new SwitchPort(null, NodeConnector.NodeConnectorIDType.OPENFLOW, "11"),
                new SwitchPort("port-10", null, null),
                new SwitchPort("port-10"),
                new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "13"),
        };

        for (SwitchPort port: ports) {
            for (short vlan : vlans) {
                PortMapConfig pmconf = new PortMapConfig(node, port, (short)vlan);
                st = mgr.setPortMap(ifp, pmconf);
                assertTrue(pmconf.toString(), st.isSuccess());

                PortMap map = null;
                try {
                    map = mgr.getPortMap(ifp);
                } catch (Exception e) {
                    unexpected(e);
                }
                assertEquals(pmconf.toString(), pmconf, map.getConfig());
                assertNotNull(pmconf.toString(), map.getNodeConnector());
                if(port.getId() != null) {
                    assertEquals(pmconf.toString(),
                            Short.parseShort(port.getId()), map.getNodeConnector().getID());
                }
                if (port.getType() != null) {
                    assertEquals(pmconf.toString(),
                            port.getType(), map.getNodeConnector().getType());
                }

                checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, pmconf.toString());
            }
        }

        st = mgr.setPortMap(ifp, null);
        assertTrue(st.isSuccess());

        PortMap map = null;
        try {
            map = mgr.getPortMap(ifp);
        } catch (Exception e) {
            unexpected(e);
        }
        assertNull(map);

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());

        // set mutli portmaps to a vbridge
        String bname1 = "vbridge1";
        VBridgePath bpath1 = new VBridgePath(tname, bname1);
        String bname2 = "vbridge2";
        VBridgePath bpath2 = new VBridgePath(tname, bname2);
        VBridgeIfPath ifp1 = new VBridgeIfPath(tname, bname1, "vinterface1");
        VBridgeIfPath ifp2 = new VBridgeIfPath(tname, bname1, "vinterface2");

        List<VBridgePath> mbpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> mifplist = new ArrayList<VBridgeIfPath>();
        mbpathlist.add(bpath1);
        mbpathlist.add(bpath2);
        mifplist.add(ifp1);
        mifplist.add(ifp2);
        createTenantAndBridgeAndInterface(mgr, tpath, mbpathlist, mifplist);

        node = NodeCreator.createOFNode(0L);
        SwitchPort port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "10");
        PortMapConfig pmconf = new PortMapConfig(node, port, (short) 0);
        st = mgr.setPortMap(ifp1, pmconf);
        assertTrue(st.isSuccess());
        checkNodeStatus(mgr, bpath1, ifp1, VNodeState.UP, VNodeState.UP, pmconf.toString());

        port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "11");
        pmconf = new PortMapConfig(node, port, (short) 0);
        st = mgr.setPortMap(ifp2, pmconf);
        assertTrue(st.isSuccess());
        checkNodeStatus(mgr, bpath1, ifp2, VNodeState.UP, VNodeState.UP, pmconf.toString());

        // in case same node connector which have diffrent definition is mapped
        node = NodeCreator.createOFNode(0L);
        port = new SwitchPort("port-10");
        pmconf = new PortMapConfig(node, port, (short) 0);
        st = mgr.setPortMap(ifp1, pmconf);
        assertTrue(st.isSuccess());
        checkNodeStatus(mgr, bpath1, ifp1, VNodeState.UP, VNodeState.UP, pmconf.toString());

        // add mapping to disabled interface.
        mgr.setPortMap(ifp1, null);
        checkNodeStatus(mgr, bpath1, ifp1, VNodeState.UP, VNodeState.UNKNOWN, pmconf.toString());

        mgr.modifyBridgeInterface(ifp1, new VInterfaceConfig(null, false), true);
        port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "10");
        pmconf = new PortMapConfig(node, port, (short) 0);
        st = mgr.setPortMap(ifp1, pmconf);
        assertTrue(st.isSuccess());
        checkNodeStatus(mgr, bpath1, ifp1, VNodeState.UP, VNodeState.DOWN, pmconf.toString());

        mgr.modifyBridgeInterface(ifp1, new VInterfaceConfig(null, true), true);
        checkNodeStatus(mgr, bpath1, ifp1, VNodeState.UP, VNodeState.UP, pmconf.toString());


        // map a internal port to interface.
        SwitchPort portIn = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "15");
        PortMapConfig pmconfIn = new PortMapConfig(node, portIn, (short) 0);
        st = mgr.setPortMap(ifp2, pmconfIn);
        assertTrue(st.isSuccess());
        checkNodeStatus(mgr, bpath1, ifp2, VNodeState.DOWN, VNodeState.DOWN, pmconfIn.toString());

        st = mgr.removeBridgeInterface(ifp2);
        assertTrue(st.isSuccess());
        checkNodeStatus(mgr, bpath1, null, VNodeState.UP, VNodeState.UP, "");

        // set duplicate portmap.
        String ifname3 = "vinterface3";
        VBridgeIfPath ifp3 = new VBridgeIfPath(tname, bname2, ifname3);
        port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "10");
        pmconf = new PortMapConfig(node, port, (short) 0);
        st = mgr.addBridgeInterface(ifp3, new VInterfaceConfig(null, true));
        st = mgr.setPortMap(ifp3, pmconf);
        assertEquals(StatusCode.CONFLICT, st.getCode());

        // remove twice
        st = mgr.setPortMap(ifp1, null);
        assertTrue(st.toString(), st.isSuccess());
        st = mgr.setPortMap(ifp1, null);
        assertTrue(st.toString(), st.isSuccess());

        // add a vlan map to this VTN.
        testVlanMapCheck(mgr, bpath1);

        // remove test settings.
        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * Common routine for {@code testVlanMap} and {@code testPortMap}
     *
     * @param mgr   a VTNManagerImpl.
     * @param bpath a VBridgePath.
     */
    private void testVlanMapCheck (VTNManagerImpl mgr, VBridgePath bpath) {
        short[] vlans = new short[] { 0, 10, 4095 };

        for (Node vnode : createNodes(3)) {
            for (short vlan : vlans) {
                VlanMapConfig vlconf = new VlanMapConfig(vnode, vlan);
                VlanMap vmap = null;
                try {
                    vmap = mgr.addVlanMap(bpath, vlconf);
                    if (vnode != null && vnode.getType() != NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        fail("throwing Exception was expected.");
                    }
                } catch (VTNException e) {
                    if (vnode != null && vnode.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        unexpected(e);
                    } else {
                     // if node type is not OF, throwing a VTNException is expected.
                        continue;
                    }
                }

                VlanMap getmap = null;
                try {
                    getmap = mgr.getVlanMap(bpath, vmap.getId());
                } catch (VTNException e) {
                    unexpected(e);
                }
                assertEquals(vlconf.toString(), getmap.getId(), vmap.getId());
                assertEquals(vlconf.toString(), getmap.getNode(), vnode);
                assertEquals(vlconf.toString(), getmap.getVlan(), vlan);

                VBridge brdg = null;
                try {
                    brdg = mgr.getBridge(bpath);
                } catch (VTNException e) {
                   unexpected(e);
                }
                assertEquals(vlconf.toString(), VNodeState.UP, brdg.getState());

                Status st = mgr.removeVlanMap(bpath, vmap.getId());
                assertTrue(vlconf.toString(), st.isSuccess());
            }
        }
    }

    /**
     * Test method for invalid cases of
     * {@link VTNManagerImpl#getPortMap(VBridgeIfPath)} and
     * {@link VTNManagerImpl#setPortMap(VBridgeIfPath, PortMapConfig)}.
     */
    @Test
    public void testPortMapInvalidCase() {
        VTNManagerImpl mgr = vtnMgr;
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname, "vinterface");

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp);
        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        Node node = NodeCreator.createOFNode(0L);
        SwitchPort port = new SwitchPort("port-1", NodeConnector.NodeConnectorIDType.OPENFLOW, "1");
        PortMapConfig pmconf = new PortMapConfig(node, port, (short) 0);
        Status st = mgr.setPortMap(ifp, pmconf);
        assertTrue(st.isSuccess());

        // bad request
        VBridgeIfPath[] biflist = new VBridgeIfPath[] {
                null, new VBridgeIfPath(tname, bname, null),
                new VBridgeIfPath(tname, null, ifp.getInterfaceName()),
                new VBridgeIfPath(null, bname, ifp.getInterfaceName())};
        SwitchPort[] badports = new SwitchPort[] {
                new SwitchPort("port-10", NodeConnector.NodeConnectorIDType.OPENFLOW, null),
                new SwitchPort("port-11", NodeConnector.NodeConnectorIDType.OPENFLOW, "10"),
                new SwitchPort(null, null, "16"),
                new SwitchPort(NodeConnector.NodeConnectorIDType.ONEPK, "10"),
        };
        PortMapConfig[] bpmlist = new PortMapConfig[] {
                new PortMapConfig(null, port, (short) 0),
                new PortMapConfig(node, null, (short) 0),
                new PortMapConfig(node, null, (short) 0),
                new PortMapConfig(node, port, (short)-1),
                new PortMapConfig(node, port, (short)4096)};

        for (VBridgeIfPath path: biflist) {
            st = mgr.setPortMap(path, pmconf);
            assertEquals((path == null) ? "null" : path.toString(),
                    StatusCode.BADREQUEST, st.getCode());

            try {
                mgr.getPortMap(path);
                fail("throwing exception was expected.");
            } catch (VTNException e) {
                assertEquals((path == null) ? "null" : path.toString(),
                        StatusCode.BADREQUEST, e.getStatus().getCode());
            }
        }

        for (SwitchPort sw: badports) {
            mgr.setPortMap(ifp, new PortMapConfig(node, sw, (short)0));
            assertEquals(sw.toString(), StatusCode.BADREQUEST, st.getCode());
        }

        for (PortMapConfig map: bpmlist) {
            st = mgr.setPortMap(ifp, map);
            assertEquals(map.toString(), StatusCode.BADREQUEST, st.getCode());
        }

        Node pnode = null;
        try {
            pnode = new Node(Node.NodeIDType.PRODUCTION, "Node ID: 0");
        } catch (ConstructionException e1) {
            unexpected(e1);
        }
        st = mgr.setPortMap(ifp, new PortMapConfig(pnode, new SwitchPort("port-1"), (short)10));
        assertEquals(StatusCode.BADREQUEST, st.getCode());

        // not found
        VBridgeIfPath[] niflist = new VBridgeIfPath[] {
                new VBridgeIfPath(tname, bname, "ii"),
                new VBridgeIfPath(tname, "bbb", ifp.getInterfaceName()),
                new VBridgeIfPath("vv", bname, ifp.getInterfaceName())};

        for (VBridgeIfPath path: niflist) {
            st = mgr.setPortMap(path, pmconf);
            assertEquals(path.toString(), StatusCode.NOTFOUND, st.getCode());

            try {
                mgr.getPortMap(path);
                fail("throwing exception was expected.");
            } catch (VTNException e) {
                assertEquals(path.toString(), StatusCode.NOTFOUND, e.getStatus().getCode());
            }
        }

        // TODO: NOTACCEPTABLE

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * Test method for
     * {@link VTNManagerImpl#notifyNode(Node, UpdateType, Map)},
     * {@link VTNManagerImpl#notifyNodeConnector(NodeConnector, UpdateType, Map)},
     * {@link VTNManagerImpl#edgeUpdate(java.util.List)}
     */
    @Test
    public void testNotificationWithPortMap() {
        VTNManagerImpl mgr = vtnMgr;
        ITopologyManager topoMgr = mgr.getTopologyManager();
        short[] vlans = new short[] { 0, 10, 4095 };
        Status st = null;
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname1 = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname1);
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname1, "vinterface");
        VBridgeIfPath difp = new VBridgeIfPath(tname, bname1, "vinterface_disabled");
        VBridgeIfPath inifp = new VBridgeIfPath(tname, bname1, "vinterfaceIn");

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp);
        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        st = mgr.addBridgeInterface(difp, new VInterfaceConfig(null, false));
        assertTrue(st.isSuccess());

        PortMap pmap = null;
        try {
            pmap = mgr.getPortMap(ifp);
        } catch (Exception e) {
            unexpected(e);
        }
        assertNull(pmap);

        Set<Node> nodeSet = new HashSet<Node>();
        Node cnode = NodeCreator.createOFNode(0L);
        Node onode = NodeCreator.createOFNode(1L);
        nodeSet.add(cnode);
        nodeSet.add(onode);

        SwitchPort[] ports = new SwitchPort[] {
                new SwitchPort("port-10", NodeConnector.NodeConnectorIDType.OPENFLOW, "10"),
                new SwitchPort(null, NodeConnector.NodeConnectorIDType.OPENFLOW, "11"),
                new SwitchPort("port-10", null, null),
                new SwitchPort("port-10"),
                new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "13"),
        };

        SwitchPort portIn = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "15");
        PortMapConfig pmconfIn = new PortMapConfig(cnode, portIn, (short) 0);
        NodeConnector ncIn
            = NodeConnectorCreator.createOFNodeConnector(Short.valueOf((short)15), cnode);

        for (Node node : nodeSet) {
            for (SwitchPort port: ports) {
                for (short vlan : vlans) {
                    PortMapConfig pmconf = new PortMapConfig(node, port, (short)vlan);
                    st = mgr.setPortMap(ifp, pmconf);
                    assertTrue(pmconf.toString(), st.isSuccess());

                    PortMap map = null;
                    try {
                        map = mgr.getPortMap(ifp);
                    } catch (Exception e) {
                        unexpected(e);
                    }
                    assertEquals(pmconf, map.getConfig());
                    assertNotNull(map.getNodeConnector());
                    if(port.getId() != null) {
                        assertEquals(pmconf.toString(),
                                Short.parseShort(port.getId()), map.getNodeConnector().getID());
                    }
                    if (port.getType() != null) {
                        assertEquals(pmconf.toString(),
                                port.getType(), map.getNodeConnector().getType());
                    }
                    checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, pmconf.toString());

                    // test each Notification APIs.
                    checkNotification(mgr, bpath, ifp, map.getNodeConnector(),
                        cnode, pmconf, null, MapType.PORT, pmconf.toString());

                    // test for a edge changed notification
                    // when port map is assigned to a internal node connector.
                    Map<Edge, Set<Property>> edges = topoMgr.getEdges();
                    for (Edge edge: edges.keySet()) {
                        List<TopoEdgeUpdate> addTopoList = new ArrayList<TopoEdgeUpdate>();
                        List<TopoEdgeUpdate> rmTopoList = new ArrayList<TopoEdgeUpdate>();

                        TopoEdgeUpdate update = new TopoEdgeUpdate(edge, null, UpdateType.ADDED);
                        addTopoList.add(update);

                        Edge reverseEdge = null;
                        try {
                            reverseEdge = new Edge(edge.getHeadNodeConnector(),
                                                    edge.getTailNodeConnector());
                        } catch (ConstructionException e) {
                            unexpected(e);
                        }
                        update = new TopoEdgeUpdate(reverseEdge, null, UpdateType.ADDED);
                        addTopoList.add(update);
                        update = new TopoEdgeUpdate(edge, null, UpdateType.REMOVED);
                        rmTopoList.add(update);
                        update = new TopoEdgeUpdate(reverseEdge, null, UpdateType.REMOVED);
                        rmTopoList.add(update);

                        st = mgr.addBridgeInterface(inifp, new VInterfaceConfig(null, null));
                        assertTrue(st.isSuccess());

                        st = mgr.setPortMap(inifp, pmconfIn);
                        assertTrue(st.isSuccess());
                        if (ncIn.equals(edge.getHeadNodeConnector()) ||
                                ncIn.equals(edge.getTailNodeConnector())) {
                            checkNodeStatus(mgr, bpath, inifp, VNodeState.DOWN, VNodeState.DOWN,
                                    pmconfIn.toString());
                            checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.UP,
                                    pmconf.toString() + "," + edge.toString());
                        } else {
                            checkNodeStatus(mgr, bpath, inifp, VNodeState.UP, VNodeState.UP,
                                    pmconfIn.toString());
                            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP,
                                    pmconf.toString() + "," + edge.toString());
                        }

                        stubObj.deleteEdge(edge);
                        stubObj.deleteEdge(reverseEdge);

                        mgr.edgeUpdate(rmTopoList);
                        checkNodeStatus(mgr, bpath, inifp, VNodeState.UP, VNodeState.UP,
                                pmconfIn.toString() + "," + edge.toString());

                        stubObj.addEdge(edge);
                        stubObj.addEdge(reverseEdge);
                        mgr.edgeUpdate(addTopoList);
                        if (ncIn.equals(edge.getHeadNodeConnector()) ||
                                ncIn.equals(edge.getTailNodeConnector())) {
                            checkNodeStatus(mgr, bpath, inifp, VNodeState.DOWN, VNodeState.DOWN,
                                    pmconfIn.toString());
                            checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.UP,
                                    pmconf.toString() + "," + edge.toString());
                        } else {
                            checkNodeStatus(mgr, bpath, inifp, VNodeState.UP, VNodeState.UP,
                                    pmconfIn.toString());
                            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP,
                                    pmconf.toString() + "," + edge.toString());
                        }

                        st = mgr.removeBridgeInterface(inifp);
                        assertTrue(st.isSuccess());

                        checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP,
                                pmconf.toString() + "," + edge.toString());

                        VInterface bif = null;
                        try {
                            bif = mgr.getBridgeInterface(difp);
                        } catch (VTNException e) {
                           unexpected(e);
                        }

                        assertEquals(false, bif.getEnabled());
                        assertEquals(VNodeState.DOWN, bif.getState());
                    }
                }
            }
        }

        // in case notified node connector is already mapped another if.
        ISwitchManager swMgr = mgr.getSwitchManager();
        String bname2 = "vbridge2";
        VBridgePath bpath2 = new VBridgePath(tname, bname2);
        VBridgeIfPath ifp2 = new VBridgeIfPath(tname, bname2, "vinterface");
        SwitchPort sp = new SwitchPort("port-10", NodeConnector.NodeConnectorIDType.OPENFLOW, "10");
        NodeConnector nc = NodeConnectorCreator.createOFNodeConnector(Short.valueOf((short)10), cnode);
        Map<String, Property> propMap = swMgr.getNodeConnectorProps(nc);
        PortMapConfig pmconf = new PortMapConfig(cnode, sp, (short)1);
        st = mgr.setPortMap(ifp, pmconf);
        assertTrue(st.toString(), st.isSuccess());
        checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, pmconf.toString());

        mgr.notifyNodeConnector(nc, UpdateType.REMOVED, propMap);

        st = mgr.addBridge(bpath2, new VBridgeConfig(null));
        assertTrue(st.toString(), st.isSuccess());
        st = mgr.addBridgeInterface(ifp2, new VInterfaceConfig(null, null));
        assertTrue(st.toString(), st.isSuccess());

        st = mgr.setPortMap(ifp2, new PortMapConfig(cnode, sp, (short)1));
        assertTrue(st.toString(), st.isSuccess());
        checkNodeStatus(mgr, bpath2, ifp2, VNodeState.DOWN, VNodeState.DOWN, pmconf.toString());

        mgr.notifyNodeConnector(nc, UpdateType.ADDED, propMap);
        mgr.notifyNodeConnector(nc, UpdateType.REMOVED, propMap);
        mgr.notifyNodeConnector(nc, UpdateType.ADDED, propMap);

        checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, pmconf.toString());
        checkNodeStatus(mgr, bpath2, ifp2, VNodeState.DOWN, VNodeState.DOWN, pmconf.toString());

        // notify disabeld node
        st = mgr.modifyBridgeInterface(ifp2, new VInterfaceConfig(null, false), true);
        assertTrue(st.toString(), st.isSuccess());

        mgr.notifyNodeConnector(nc, UpdateType.REMOVED, propMap);
        mgr.notifyNodeConnector(nc, UpdateType.ADDED, propMap);

        checkNodeStatus(mgr, bpath2, ifp2, VNodeState.UNKNOWN, VNodeState.DOWN, pmconf.toString());

        // if mapped port is null.
        sp = new SwitchPort("port-16", NodeConnector.NodeConnectorIDType.OPENFLOW, "16");
        pmconf = new PortMapConfig(cnode, sp, (short)0);
        st = mgr.setPortMap(ifp, pmconf);
        propMap = null;
        mgr.notifyNode(cnode, UpdateType.REMOVED, propMap);
        checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.DOWN,
                pmconf.toString());

        nc = NodeConnectorCreator.createOFNodeConnector((short)16, cnode);
        propMap = swMgr.getNodeConnectorProps(nc);
        mgr.notifyNodeConnector(nc, UpdateType.CHANGED, propMap);
        checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.DOWN,
                pmconf.toString());

        mgr.edgeUpdate(null);
        checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.DOWN,
                pmconf.toString());

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * same test with {@code testNotifyNodeAndNodeConnectorWithPortMap}.
     * this test case do with vlan map setting.
     */
    @Test
    public void testNotificationWithVlanMap() {
        VTNManagerImpl mgr = vtnMgr;
        Set<Node> nodeSet = new HashSet<Node>();
        short[] vlans = new short[] { 0, 10, 4095 };

        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        String ifname = "vinterface";
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname, ifname);
        Status st = null;

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp);
        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        Node cnode = NodeCreator.createOFNode(0L);
        Node onode = NodeCreator.createOFNode(1L);
        nodeSet.add(null);
        nodeSet.add(cnode);
        nodeSet.add(onode);

        NodeConnector nc
            = NodeConnectorCreator.createOFNodeConnector(Short.valueOf((short)10), cnode);

        // add a vlanmap to a vbridge
        for (Node node : nodeSet) {
            for (short vlan : vlans) {
                VlanMapConfig vlconf = new VlanMapConfig(node, vlan);
                VlanMap map = null;
                try {
                    map = mgr.addVlanMap(bpath, vlconf);
                    if (node != null && node.getType() != NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        fail("throwing Exception was expected.");
                    }
                } catch (VTNException e) {
                    if (node != null && node.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        unexpected(e);
                    } else {
                        continue;
                    }
                }

                VlanMap getmap = null;
                try {
                    getmap = mgr.getVlanMap(bpath, map.getId());
                } catch (VTNException e) {
                    unexpected(e);
                }
                assertEquals(getmap.getId(), map.getId());
                assertEquals(getmap.getNode(), node);
                assertEquals(getmap.getVlan(), vlan);
                checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, vlconf.toString());

                // test for notification APIs.
                checkNotification (mgr, bpath, ifp, nc,
                        cnode, null, vlconf, MapType.VLAN, vlconf.toString());

                st = mgr.removeVlanMap(bpath, map.getId());
                assertTrue(st.isSuccess());
            }
        }

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    @Test
    public void  testNotificationWithBothMapped() {
        VTNManagerImpl mgr = vtnMgr;
        Set<Node> nodeSet = new HashSet<Node>();

        short[] vlans = new short[] { 0, 10, 4095 };

        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        String ifname = "vinterface";
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname, ifname);
        Status st = null;

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp);
        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        PortMap pmap = null;
        try {
            pmap = mgr.getPortMap(ifp);
        } catch (Exception e) {
            unexpected(e);
        }
        assertNull(pmap);

        Node pnode = NodeCreator.createOFNode(0L);
        Node onode = NodeCreator.createOFNode(1L);
        nodeSet.add(null);
        nodeSet.add(pnode);
        nodeSet.add(onode);

        SwitchPort[] ports = new SwitchPort[] {
                new SwitchPort("port-10", NodeConnector.NodeConnectorIDType.OPENFLOW, "10"),
                new SwitchPort(null, NodeConnector.NodeConnectorIDType.OPENFLOW, "11"),
                new SwitchPort("port-10", null, null),
                new SwitchPort("port-10"),
                new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW, "13"),
        };

        Node cnode = NodeCreator.createOFNode(0L);
        NodeConnector cnc
            = NodeConnectorCreator.createOFNodeConnector(Short.valueOf((short)10), cnode);

        for (Node pmapNode : nodeSet) {
            if (pmapNode == null) {
                continue;
            }
            for (SwitchPort port: ports) {
                for (short vlan : vlans) {
                    // add port map
                    PortMapConfig pmconf = new PortMapConfig(pmapNode, port, (short)vlan);
                    st = mgr.setPortMap(ifp, pmconf);
                    assertTrue(pmconf.toString(), st.isSuccess());

                    PortMap map = null;
                    try {
                        map = mgr.getPortMap(ifp);
                    } catch (Exception e) {
                        unexpected(e);
                    }
                    assertEquals(pmconf, map.getConfig());
                    assertNotNull(map.getNodeConnector());
                    if(port.getId() != null) {
                        assertEquals(pmconf.toString(),
                            Short.parseShort(port.getId()), map.getNodeConnector().getID());
                    }
                    if (port.getType() != null) {
                        assertEquals(pmconf.toString(),
                            port.getType(), map.getNodeConnector().getType());
                    }
                    checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, pmconf.toString());

                    // add a vlanmap to a vbridge
                    for (Node vmapNode : nodeSet) {
                        for (short vvlan : vlans) {
                            VlanMapConfig vlconf = new VlanMapConfig(vmapNode, vvlan);
                            VlanMap vmap = null;
                            try {
                                vmap = mgr.addVlanMap(bpath, vlconf);
                                if (vmapNode != null && vmapNode.getType() != NodeConnector.NodeConnectorIDType.OPENFLOW) {
                                    fail("throwing Exception was expected.");
                                }
                            } catch (VTNException e) {
                                if (vmapNode != null && vmapNode.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                                    unexpected(e);
                                } else {
                                    continue;
                                }
                            }

                            VlanMap getmap = null;
                            try {
                                getmap = mgr.getVlanMap(bpath, vmap.getId());
                            } catch (VTNException e) {
                                unexpected(e);
                            }
                            assertEquals(getmap.getId(), vmap.getId());
                            assertEquals(getmap.getNode(), vmapNode);
                            assertEquals(getmap.getVlan(), vvlan);
                            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, vlconf.toString());

                            // test for notification APIs.
                            checkNotification (mgr, bpath, ifp, cnc,
                                cnode, pmconf, vlconf, MapType.ALL,
                                pmconf.toString() + "," + vlconf.toString());

                            st = mgr.removeVlanMap(bpath, vmap.getId());
                            assertTrue(st.isSuccess());
                        }
                    }
                }
            }
        }

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * do test of Notification APIs.
     *
     * @param mgr   VTNManagerImpl
     * @param bpath VBridgePath
     * @param ifp   VBridgeIfPath
     * @param nc    NodeConnector which status changed
     * @param otherNc   NodeConnector which status changed but not match mapped one.
     * @param node  Node which status changed
     * @param onode Node which status changed but not match mapped one.
     * @param pmconf   PortMapConfig. If no portmap, specify null.
     * @param vlconf    VlanMapConfig. If no vlanmap, specify null.
     * @param mapType   MapType
     * @param msg   this message output when assertion is failed.
     */
    private void checkNotification (VTNManagerImpl mgr, VBridgePath bpath, VBridgeIfPath ifp,
            NodeConnector chgNc, Node chgNode, PortMapConfig pmconf, VlanMapConfig vlconf,
            MapType mapType, String msg) {
        ISwitchManager swMgr = mgr.getSwitchManager();
        ITopologyManager topoMgr = mgr.getTopologyManager();

        NodeConnector mapNc = null;
        Node portMapNode = null;
        Node vlanMapNode = null;
        if (pmconf != null) {
            Short s;
            if (pmconf.getPort().getId() == null) {
                String [] tkn = pmconf.getPort().getName().split("-");
                s = Short.valueOf(tkn[1]);
            } else {
                s = Short.valueOf(pmconf.getPort().getId());
            }
            mapNc = NodeConnectorCreator.createOFNodeConnector(s, pmconf.getNode());
            portMapNode = mapNc.getNode();
        }
        if (vlconf != null) {
            vlanMapNode = vlconf.getNode();
        }

        // test for nodeconnector change notify.
        Map<String, Property> propMap = null; // not used now.
        checkMacTableEntry(mgr, bpath, true, msg);
        putMacTableEntry(mgr, bpath, chgNc);
        mgr.notifyNodeConnector(chgNc, UpdateType.REMOVED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            if (chgNc.equals(mapNc)) {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.DOWN, msg);
            } else {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
            }
        } else {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, msg);
        }
        if (chgNc.equals(mapNc) || vlanMapNode == null || chgNode.equals(vlanMapNode)) {
            checkMacTableEntry(mgr, bpath, true, msg);
        } else {
            checkMacTableEntry(mgr, bpath, false, msg);
        }

        propMap = swMgr.getNodeConnectorProps(chgNc);
        mgr.notifyNodeConnector(chgNc, UpdateType.ADDED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
        } else {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, msg);
        }

        // add again
        mgr.notifyNodeConnector(chgNc, UpdateType.ADDED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
        } else {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, msg);
        }

        mgr.notifyNodeConnector(chgNc, UpdateType.CHANGED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
        } else {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, msg);
        }
        flushMacTableEntry(mgr, bpath);

        // test for node change notify.
        putMacTableEntry(mgr, bpath, chgNc);
        mgr.notifyNode(chgNode, UpdateType.REMOVED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            if (chgNode.equals(portMapNode)) {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.DOWN, msg);
                checkMacTableEntry(mgr, bpath, true, msg);
            } else if (chgNode.equals(vlanMapNode)) {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.UP, msg);
                checkMacTableEntry(mgr, bpath, true, msg);
            } else {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
                checkMacTableEntry(mgr, bpath,
                        (mapType.equals(MapType.ALL) && vlanMapNode == null) ? true : false, msg);
            }
        } else {
            if (chgNode.equals(vlanMapNode)) {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.UNKNOWN,msg);
                checkMacTableEntry(mgr, bpath, true, msg);
            } else {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN,msg);
                checkMacTableEntry(mgr, bpath, (vlanMapNode == null) ? true : false, msg);
            }
        }
        flushMacTableEntry(mgr, bpath);

        mgr.initInventory();
        mgr.getNodeDB().remove(chgNode);
        mgr.notifyNode(chgNode, UpdateType.ADDED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            if (chgNode.equals(portMapNode)) {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.DOWN, VNodeState.DOWN, msg);
            } else {
                checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
            }
        } else {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, msg);
        }

        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            mgr.initInventory();
            mgr.getPortDB().remove(mapNc);
            propMap = swMgr.getNodeConnectorProps(mapNc);
            mgr.notifyNodeConnector(mapNc, UpdateType.ADDED, propMap);
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
        }

        mgr.notifyNode(chgNode, UpdateType.CHANGED, propMap);
        if (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UP, msg);
        } else {
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP, VNodeState.UNKNOWN, msg);
        }

        // test for a edge changed notification
        Map<Edge, Set<Property>> edges = topoMgr.getEdges();
        for (Edge edge: edges.keySet()) {
            List<TopoEdgeUpdate> addTopoList = new ArrayList<TopoEdgeUpdate>();
            List<TopoEdgeUpdate> rmTopoList = new ArrayList<TopoEdgeUpdate>();
            List<TopoEdgeUpdate> chgTopoList = new ArrayList<TopoEdgeUpdate>();

            TopoEdgeUpdate update = new TopoEdgeUpdate(edge, null, UpdateType.ADDED);
            addTopoList.add(update);

            Edge reverseEdge = null;
            try {
                reverseEdge = new Edge(edge.getHeadNodeConnector(),
                        edge.getTailNodeConnector());
            } catch (ConstructionException e) {
                unexpected(e);
            }
            update = new TopoEdgeUpdate(reverseEdge, null, UpdateType.ADDED);
            addTopoList.add(update);

            mgr.edgeUpdate(addTopoList);
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP,
                    (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) ? VNodeState.UP : VNodeState.UNKNOWN,
                    msg + "," + edge.toString());

            update = new TopoEdgeUpdate(edge, null, UpdateType.REMOVED);
            rmTopoList.add(update);
            update = new TopoEdgeUpdate(reverseEdge, null, UpdateType.REMOVED);
            rmTopoList.add(update);

            mgr.edgeUpdate(rmTopoList);
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP,
                    (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) ? VNodeState.UP : VNodeState.UNKNOWN,
                    msg + "," + edge.toString());

            update = new TopoEdgeUpdate(edge, null, UpdateType.CHANGED);
            chgTopoList.add(update);
            update = new TopoEdgeUpdate(reverseEdge, null, UpdateType.CHANGED);
            chgTopoList.add(update);

            mgr.edgeUpdate(chgTopoList);
            checkNodeStatus(mgr, bpath, ifp, VNodeState.UP,
                    (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) ? VNodeState.UP : VNodeState.UNKNOWN,
                    msg + "," + edge.toString());
        }

        // a edge consist of unmapped nodeconnector updated
        Node node = NodeCreator.createOFNode(Long.valueOf(10));
        NodeConnector head = NodeConnectorCreator.createOFNodeConnector(Short.valueOf((short)10), node);
        NodeConnector tail = NodeConnectorCreator.createOFNodeConnector(Short.valueOf((short)11), node);
        Edge edge = null;
        try {
             edge = new Edge(head, tail);
        } catch (ConstructionException e) {
            unexpected(e);
        }
        List<TopoEdgeUpdate> topoList = new ArrayList<TopoEdgeUpdate>();
        TopoEdgeUpdate update = new TopoEdgeUpdate(edge, null, UpdateType.ADDED);
        topoList.add(update);
        mgr.edgeUpdate(topoList);
        checkNodeStatus(mgr, bpath, ifp, VNodeState.UP,
                (mapType.equals(MapType.PORT) || mapType.equals(MapType.ALL)) ? VNodeState.UP : VNodeState.UNKNOWN,
                msg + "," + edge.toString());

        // Reset inventory cache.
        mgr.initInventory();
    }


    /**
     * check node and interface status specified vbridge and vinterface.
     * @param mgr   VTNManagerImpl
     * @param bpath checked VBridgePath
     * @param ifp   checked VBridgeIfPath
     * @param bstate expected vbridge state
     * @param ifstate expected vinterface state
     * @param msg message strings print when assertion failed.
     */
    private void checkNodeStatus(VTNManagerImpl mgr, VBridgePath bpath, VBridgeIfPath ifp,
            VNodeState bstate, VNodeState ifstate, String msg) {

        VBridge brdg = null;
        VInterface bif = null;
        try {
            if (ifp != null) {
                bif = mgr.getBridgeInterface(ifp);
            }
            if (bpath != null) {
                brdg = mgr.getBridge(bpath);
            }
        } catch (VTNException e) {
            unexpected(e);
        }

        if (ifp != null) {
            assertEquals("VBridgeInterface status: " + msg, ifstate, bif.getState());
        }
        if (bpath != null) {
            assertEquals("VBridge status: " + msg, bstate, brdg.getState());
        }
     }

    /**
     * put a Mac Address Table Entry to Mac Address Table of specified bridge.
     *
     * @param mgr   VTNManagerImpl
     * @param bpath VBridgePath
     * @param nc    NodeConnector
     */
    private void putMacTableEntry(VTNManagerImpl mgr, VBridgePath bpath, NodeConnector nc) {
        byte[] src = new byte[] {(byte)0x00, (byte)0x01, (byte)0x01, (byte)0x01, (byte)0x01, (byte)0x01,};
        byte[] dst = new byte[] {(byte)0xFF, (byte)0xFF, (byte)0xFF, (byte)0xFF, (byte)0xFF, (byte)0xFF};
        byte[] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)1};
        byte[] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};

        PacketContext pctx = createARPPacketContext(src, dst, sender ,target,
                                                    (short)-1, nc, ARP.REQUEST);
        MacAddressTable tbl = mgr.getMacAddressTable(bpath);
        tbl.add(pctx);
    }

    /**
     * check a Mac Address Table Entry.
     *
     * @param mgr   VTNManagerImpl
     * @param bpath VBridgePath
     * @param isFlushed if true, expected result is 0. if not 0, execpted result is more than 0.
     * @param msg
     */
    private void checkMacTableEntry(VTNManagerImpl mgr, VBridgePath bpath, boolean isFlushed, String msg) {
        MacAddressTable tbl = mgr.getMacAddressTable(bpath);

        List<MacAddressEntry> list = null;
        try {
            list = tbl.getEntries();
        } catch (VTNException e) {
           unexpected(e);
        }

        if (isFlushed) {
            assertEquals(msg, 0, list.size());
        } else {
            assertTrue(msg, list.size() > 0);
        }
    }

    /**
     * flush Mac Table
     * @param mgr VTNManagerImpl
     * @param bpath VBridgePath
     */
    private void flushMacTableEntry(VTNManagerImpl mgr, VBridgePath bpath) {
        mgr.flushMacEntries(bpath);
    }

    /**
     * test for
     * {@link VTNManagerImpl#recalculateDone()}
     */
    @Test
    public void testRecalculate() {
        VTNManagerImpl mgr = vtnMgr;
        TestStub stub0 = new TestStub(0);
        TestStub stub3 = new TestStub(3);
        VTNManagerAwareStub awareStub = new VTNManagerAwareStub();

        mgr.setSwitchManager(stub3);
        mgr.setTopologyManager(stub3);
        mgr.setRouting(stub0);
        mgr.initInventory();
        mgr.clearDisabledNode();

        // setup vbridge
        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname = "vbridge";
        VBridgePath bpath = new VBridgePath(tname, bname);
        String ifname0 = "vinterface0";
        VBridgeIfPath ifp0 = new VBridgeIfPath(tname, bname, ifname0);
        String ifname1 = "vinterface1";
        VBridgeIfPath ifp1 = new VBridgeIfPath(tname, bname, ifname1);
        String ifname2 = "vinterface2";
        VBridgeIfPath ifp2 = new VBridgeIfPath(tname, bname, ifname2);

        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        List<VBridgeIfPath> ifplist = new ArrayList<VBridgeIfPath>();
        bpathlist.add(bpath);
        ifplist.add(ifp0);
        ifplist.add(ifp1);
        ifplist.add(ifp2);
        createTenantAndBridgeAndInterface(mgr, tpath, bpathlist, ifplist);

        // Append one more listener to catch port mapping change events.
        VTNManagerAwareStub pmapAware = new VTNManagerAwareStub();
        mgr.addVTNManagerAware(pmapAware);

        Node node0 = NodeCreator.createOFNode(0L);
        SwitchPort port = new SwitchPort(null, NodeConnector.NodeConnectorIDType.OPENFLOW, "10");
        PortMapConfig pmconf = new PortMapConfig(node0, port, (short) 0);
        Status st = mgr.setPortMap(ifp0, pmconf);
        assertTrue(st.isSuccess());
        Node node1 = NodeCreator.createOFNode(1L);
        pmconf = new PortMapConfig(node1, port, (short) 0);
        st = mgr.setPortMap(ifp1, pmconf);
        assertTrue(st.isSuccess());
        Node node2 = NodeCreator.createOFNode(2L);
        pmconf = new PortMapConfig(node2, port, (short) 0);
        st = mgr.setPortMap(ifp2, pmconf);
        assertTrue(st.isSuccess());

        // Ensure that no port mapping change event is delivered to awareStub.
        pmapAware.checkPmapInfo(3, ifp2, pmconf, UpdateType.ADDED);
        mgr.removeVTNManagerAware(pmapAware);

        mgr.addVTNManagerAware(awareStub);
        awareStub.checkVtnInfo(1, tpath, tname, UpdateType.ADDED);
        awareStub.checkVbrInfo(1, bpath, bname, UpdateType.ADDED);
        awareStub.checkVIfInfo(3, ifp2, ifname2, UpdateType.ADDED);
        awareStub.checkPmapInfo(3, ifp2, pmconf, UpdateType.ADDED);

        // call with no fault status
        mgr.recalculateDone();

        // add fault path
        byte[] src0 = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                  (byte)0x11, (byte)0x11, (byte)0x11};
        byte[] src1 = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                  (byte)0x22, (byte)0x22, (byte)0x22};
        byte[] src2 = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                   (byte)0x33, (byte)0x33, (byte)0x33};
        byte[] dst = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                 (byte)0xff, (byte)0xff, (byte)0xff};
        byte[] sender1 = new byte[] {(byte)192, (byte)168, (byte)0, (byte)1};
        byte[] sender2 = new byte[] {(byte)192, (byte)168, (byte)0, (byte)2};
        byte[] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};
        short vlan = 0;

        NodeConnector nc1
            = NodeConnectorCreator.createOFNodeConnector(Short.valueOf("10"), node1);
        RawPacket inPkt = createARPRawPacket(src1, dst, sender1, target, (vlan > 0) ? vlan : -1,
                                                nc1, ARP.REQUEST);
        PacketResult result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);

        NodeConnector nc2
            = NodeConnectorCreator.createOFNodeConnector(Short.valueOf("10"), node2);
        inPkt = createARPRawPacket(src2, dst, sender2, target, (vlan > 0) ? vlan : -1, nc2, ARP.REQUEST);
        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);

        NodeConnector nc0
            = NodeConnectorCreator.createOFNodeConnector(Short.valueOf("10"), node0);
        inPkt = createARPRawPacket(src0, src1, target, sender1, (vlan > 0) ? vlan : -1, nc0, ARP.REPLY);
        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);
        awareStub.checkVbrInfo(1, bpath, bname, UpdateType.CHANGED);

        inPkt = createARPRawPacket(src0, src2, target, sender2, (vlan > 0) ? vlan : -1, nc0, ARP.REPLY);
        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);
        awareStub.checkVbrInfo(1, bpath, bname, UpdateType.CHANGED);
        checkVBridgeStatus(mgr, bpath, 2, VNodeState.DOWN);

        // received again. this expect status is keeped
        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);
        awareStub.checkVbrInfo(0, null, null, null);

        pmconf = new PortMapConfig(node2, port, (short) 0);
        st = mgr.setPortMap(ifp2, pmconf);
        checkVBridgeStatus(mgr, bpath, 2, VNodeState.DOWN);

        // expected not to recover from fault state
        mgr.recalculateDone();
        checkVBridgeStatus(mgr, bpath, 2, VNodeState.DOWN);
        awareStub.checkVbrInfo(0, null, null, null);

        // change path settings
        mgr.setRouting(stubObj);

        // expected to 1 path recover from fault state
        mgr.recalculateDone();
        checkVBridgeStatus(mgr, bpath, 1, VNodeState.DOWN);
        awareStub.checkVbrInfo(1, bpath, bname, UpdateType.CHANGED);

        // change path settings
        mgr.setRouting(stub3);

        // expected to recover from fault state
        mgr.recalculateDone();
        checkVBridgeStatus(mgr, bpath, 0, VNodeState.UP);
        awareStub.checkVbrInfo(1, bpath, bname, UpdateType.CHANGED);

        mgr.setSwitchManager(stubObj);
        mgr.setTopologyManager(stubObj);
        mgr.setRouting(stubObj);
        mgr.initInventory();
        mgr.clearDisabledNode();

        st = mgr.removeTenant(tpath);
        assertTrue(st.toString(), st.isSuccess());
    }

    /**
     *  check VBridgeStatus.
     *  (used in testRecalculate())
     *
     * @param mgr       a reference to VTNManagerImpl.
     * @param bpath     a VBridgePath object.
     * @param faults    number of fault path.
     * @param state     a state of VBridge expected.
     */
    private void checkVBridgeStatus(VTNManagerImpl mgr, VBridgePath bpath, int faults, VNodeState state) {
        VBridge brdg = null;
        try {
            brdg = mgr.getBridge(bpath);
        } catch (VTNException e) {
            unexpected(e);
        }
        VNodeState bstate = brdg.getState();
        assertEquals(faults, brdg.getFaults());
        assertEquals(state, bstate);
    }

    /**
     * Test method for {@link VTNManagerImpl#receiveDataPacket(RawPacket)}.
     * The case PortMap applied to vBridge.
     */
    @Test
    public void testReceiveDataPacketPortMapped() {
        VTNManagerImpl mgr = vtnMgr;
        TestStub stub = stubObj;
        ISwitchManager swmgr = mgr.getSwitchManager();
        short[] vlans = new short[] { 0, 10, 4095 };
        byte[] cntMac = swmgr.getControllerMAC();
        ConcurrentMap<VBridgePath, Set<NodeConnector>> mappedConnectors
            = new ConcurrentHashMap<VBridgePath, Set<NodeConnector>>();
        Status st = null;

        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname1 = "vbridge1";
        VBridgePath bpath1 = new VBridgePath(tname, bname1);
        String bname2 = "vbridge2";
        VBridgePath bpath2 = new VBridgePath(tname, bname2);
        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        bpathlist.add(bpath1);
        bpathlist.add(bpath2);
        createTenantAndBridge(mgr, tpath, bpathlist);

        mappedConnectors.put(bpath1, new HashSet<NodeConnector>());
        mappedConnectors.put(bpath2, new HashSet<NodeConnector>());

        Set<Node> existNodes = swmgr.getNodes();
        Set<NodeConnector> existConnectors = new HashSet<NodeConnector>();
        for (Node node: existNodes) {
            existConnectors.addAll(swmgr.getNodeConnectors(node));
        }

        // add interface
        for (short i = 0; i < 4; i++) {
            int inode = 0;
            for (Node node : existNodes) {
                String ifname = "vinterface" + inode + (i + 10);
                VBridgeIfPath ifp = new VBridgeIfPath(tname, (i < 2) ? bname1 : bname2,
                                                    ifname);
                st = mgr.addBridgeInterface(ifp, new VInterfaceConfig(null, true));
                assertTrue(ifp.toString(), st.isSuccess());
                inode++;
            }
        }

        // null case
        PacketResult result = mgr.receiveDataPacket(null);
        assertEquals(PacketResult.IGNORED, result);

        for (short vlan: vlans) {
            // add port mapping
            Set<PortVlan> mappedThis = new HashSet<PortVlan>();

            for (short i = 0; i < 4; i++) {
                int inode = 0;
                for (Node node : existNodes) {
                    short setvlan = (i != 1) ? vlan :100;
                    String ifname = "vinterface" + inode + (i + 10);
                    VBridgeIfPath ifp = new VBridgeIfPath(tname, (i < 2) ? bname1 : bname2,
                                                            ifname);

                    SwitchPort port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW,
                                                    String.valueOf(i + 10));
                    PortMapConfig pmconf = new PortMapConfig(node, port, setvlan);
                    st = mgr.setPortMap(ifp, pmconf);
                    assertTrue(ifp.toString() + "," + pmconf.toString(), st.isSuccess());

                    NodeConnector mapnc = NodeConnectorCreator.createOFNodeConnector(
                                                        Short.valueOf((short)(i + 10)), node);
                    Set<NodeConnector> set  = mappedConnectors.get((i < 2)? bpath1 : bpath2);
                    set.add(mapnc);
                    mappedConnectors.put((i < 2)? bpath1 : bpath2, set);
                    if (i < 2) {
                        mappedThis.add(new PortVlan(mapnc, setvlan));
                    }

                    inode++;
                }
            }

            testReceiveDataPacketBCLoop(mgr, bpath1, MapType.PORT, mappedThis, stub);

            st = mgr.flushMacEntries(bpath1);
            assertTrue("vlan=" + vlan, st.isSuccess());

//            testReceiveDataPacketUCLoopLearned(mgr, bpath1, MapType.PORT, mappedThis, stub);
//            st = mgr.flushMacEntries(bpath1);
//            assertTrue("vlan=" + vlan, st.isSuccess());

            testReceiveDataPacketUCLoop(mgr, bpath1, MapType.PORT, mappedThis, stub);

            st = mgr.flushMacEntries(bpath1);
            assertTrue("vlan=" + vlan, st.isSuccess());

            testReceiveDataPacketUCLoopLearned(mgr, bpath1, MapType.PORT, mappedThis, stub);


            // in case recieved a packet from controller.
            for (PortVlan pv: mappedThis) {
                byte [] dst = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                        (byte)0xff, (byte)0xff, (byte)0xff};
                byte [] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)1};
                byte [] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};
                NodeConnector nc = pv.getNodeConnector();

                RawPacket inPkt = createARPRawPacket(cntMac, dst, sender, target,
                                                (pv.getVlan() > 0) ? pv.getVlan() : -1,
                                                        nc, ARP.REQUEST);
                result = mgr.receiveDataPacket(inPkt);
                assertEquals(nc.toString(), PacketResult.IGNORED, result);

                st = mgr.flushMacEntries(bpath1);
                assertTrue(nc.toString(), st.isSuccess());
            }
        }

        st = mgr.removeBridge(bpath2);
        assertTrue(st.isSuccess());

        // in case received pacekt from no mapped port.
        byte[] dst = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                (byte)0xff, (byte)0xff, (byte)0xff};
        byte[] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)1};
        byte[] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};

        for (NodeConnector nc: mappedConnectors.get(bpath2)) {
            byte iphost = 1;
            for (EthernetAddress ea: createEthernetAddresses(false)) {
                byte [] bytes = ea.getValue();
                byte [] src = new byte[] {bytes[0], bytes[1], bytes[2],
                        bytes[3], bytes[4], bytes[5]};
                sender[3] = iphost;
                RawPacket inPkt = createARPRawPacket(src, dst, sender, target, (short)-1, nc, ARP.REQUEST);

                result = mgr.receiveDataPacket(inPkt);
                if (nc != null) {
                    assertEquals(nc.toString() + "," + ea.toString(), PacketResult.IGNORED, result);
                } else {
                    assertEquals(ea.toString(), PacketResult.IGNORED, result);
                }

                List<RawPacket> transDatas = stub.getTransmittedDataPacket();
                assertEquals(nc.toString() + "," + ea.toString(), 0, transDatas.size());
                iphost++;
            }
        }

        // in case received packet from internal port.
        byte[] src = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                 (byte)0x11, (byte)0x11, (byte)0x11};
        Node node = NodeCreator.createOFNode(0L);
        NodeConnector innc = NodeConnectorCreator.createOFNodeConnector(
                Short.valueOf((short)15), node);
        RawPacket inPkt = createARPRawPacket(src, dst, sender, target, (short)-1, innc, ARP.REQUEST);

        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.IGNORED, result);
        List<RawPacket> transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // not Ethernet Packet
        Ethernet inPktDecoded = (Ethernet)stub.decodeDataPacket(inPkt);
        ARP arp = (ARP)inPktDecoded.getPayload();
        RawPacket arpPkt = stub.encodeDataPacket(arp);
        NodeConnector nc = NodeConnectorCreator.createOFNodeConnector(
                Short.valueOf((short)10), node);
        arpPkt.setIncomingNodeConnector(nc);

        result = mgr.receiveDataPacket(arpPkt);
        assertEquals(PacketResult.IGNORED, result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // in case out PortVlan is not match.
        byte[] src2 = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                (byte)0x11, (byte)0x11, (byte)0x12};
        MacAddressTable table = mgr.getMacAddressTable(bpath1);
        PacketContext pctx = createARPPacketContext(src, dst, sender, target,
                (short) 99, nc, ARP.REQUEST);
        table.flush();
        table.add(pctx);

        VBridgeIfPath ifp1 = new VBridgeIfPath(tname, bname1, "vinterface010");
        PortMap map = null;
        try {
            map = mgr.getPortMap(ifp1);
        } catch (VTNException e) {
            unexpected(e);
        }
        short confVlan = map.getConfig().getVlan();
        inPkt = createIPv4RawPacket(src2, src, target, sender,
                (confVlan > 0) ? confVlan : -1, map.getNodeConnector());
        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);
        transDatas = stub.getTransmittedDataPacket();
        // receive flood + probe packet to incoming nodeconnector.
        assertEquals(mappedConnectors.get(bpath1).size(), transDatas.size());
        MacTableEntry ent = table.get(pctx);
        assertNull(ent);

        // in case out interface is disabled.

        VBridgeIfPath ifp2 = new VBridgeIfPath(tname, bname1, "vinterface011");
        map = null;
        try {
            map = mgr.getPortMap(ifp2);
        } catch (VTNException e) {
            unexpected(e);
        }
        confVlan = map.getConfig().getVlan();

        st = mgr.modifyBridgeInterface(ifp1,
                new VInterfaceConfig(null, Boolean.FALSE), true);

        inPkt = createIPv4RawPacket(src, src2, sender, target,
                (confVlan > 0) ? confVlan : -1, map.getNodeConnector());
        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(1, transDatas.size());

        st = mgr.modifyBridgeInterface(ifp1,
                new VInterfaceConfig(null, Boolean.TRUE), true);

        table.flush();

        // in case dst is controller
        inPkt = createARPRawPacket(src, cntMac, sender, target,
                (confVlan > 0) ? confVlan : -1, map.getNodeConnector(), ARP.REQUEST);

        result = mgr.receiveDataPacket(inPkt);
        assertEquals(PacketResult.KEEP_PROCESSING, result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // in case received disable vinterface.
        st = mgr.modifyBridgeInterface(ifp1,
                new VInterfaceConfig(null, Boolean.FALSE), true);
        assertTrue(st.toString(), st.isSuccess());
        try {
            map = mgr.getPortMap(ifp1);
        } catch (VTNException e) {
            unexpected(e);
        }
        confVlan = map.getConfig().getVlan();

        inPkt = createARPRawPacket(src, dst, sender, target,
                (confVlan > 0) ? confVlan : -1, map.getNodeConnector(), ARP.REQUEST);
        result = mgr.receiveDataPacket(inPkt);
        // TODO: need to check
//        assertEquals(PacketResult.IGNORED, result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // remove tenant after test
        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * Test method for {@link VTNManagerImpl#receiveDataPacket(RawPacket)}.
     * The case VlanMap applied to vBridge.
     */
    @Test
    public void testReceiveDataPacketVlanMapped() {
        VTNManagerImpl mgr = vtnMgr;
        TestStub stub = stubObj;
        ISwitchManager swmgr = mgr.getSwitchManager();
        ITopologyManager topomgr = mgr.getTopologyManager();
        short[] vlans = new short[] { 0, 10, 4095 };
        byte[] cntMac = swmgr.getControllerMAC();
        Status st = null;

        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname1 = "vbridge1";
        VBridgePath bpath1 = new VBridgePath(tname, bname1);
        String bname2 = "vbridge2";
        VBridgePath bpath2 = new VBridgePath(tname, bname2);
        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        bpathlist.add(bpath1);
        bpathlist.add(bpath2);
        createTenantAndBridge(mgr, tpath, bpathlist);

        Set<Node> existNodes = swmgr.getNodes();
        Set<NodeConnector> existConnectors = new HashSet<NodeConnector>();
        for (Node node: existNodes) {
            existConnectors.addAll(swmgr.getNodeConnectors(node));
        }

        Set<Node> loopNodes = new HashSet<Node>(existNodes);
        loopNodes.add(null);
        for (Node mapNode : loopNodes) {
            for (short vlan : vlans) {
                VlanMapConfig vlconf = new VlanMapConfig(mapNode, vlan);
                VlanMap map = null;
                try {
                    map = mgr.addVlanMap(bpath1, vlconf);
                } catch (VTNException e) {
                    unexpected(e);
                }

                Set<PortVlan> mappedThis = new HashSet<PortVlan>();
                for (Node node: existNodes) {
                    if (mapNode == null || node.equals(mapNode)) {
                        for (NodeConnector nc : swmgr.getNodeConnectors(node)) {
                            if (!topomgr.isInternal(nc)) {
                                mappedThis.add(new PortVlan(nc, vlan));
                            }
                        }
                    }
                }

                testReceiveDataPacketBCLoop(mgr, bpath1, MapType.PORT, mappedThis, stub);

                st = mgr.flushMacEntries(bpath1);
                assertTrue(vlconf.toString(), st.isSuccess());

//                testReceiveDataPacketUCLoopLearned(mgr, bpath1, MapType.PORT, mappedThis, stub);

//                st = mgr.flushMacEntries(bpath1);
//                assertTrue(vlconf.toString(), st.isSuccess());

                testReceiveDataPacketUCLoop(mgr, bpath1, MapType.PORT, mappedThis, stub);

                st = mgr.flushMacEntries(bpath1);
                assertTrue(vlconf.toString(), st.isSuccess());

                testReceiveDataPacketUCLoopLearned(mgr, bpath1, MapType.VLAN, mappedThis, stub);

                // in case received from non-mapped port.
                for (Node unmapNode : loopNodes) {
                    if (unmapNode != null && unmapNode.equals(mapNode)) {
                        byte[] src = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                (byte)0x11, (byte)0x11, (byte)0x11};
                        byte[] dst = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                (byte)0xff, (byte)0xff, (byte)0xff};
                        byte[] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)1};
                        byte[] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};
                        NodeConnector innc = NodeConnectorCreator
                                .createOFNodeConnector(Short.valueOf((short)10), unmapNode);
                        RawPacket inPkt = createARPRawPacket(src, cntMac, sender, target,
                                (short)99, innc, ARP.REQUEST);

                        PacketResult result = mgr.receiveDataPacket(inPkt);
                    }
                }

                st = mgr.removeVlanMap(bpath1, map.getId());
                assertTrue(vlconf.toString(), st.isSuccess());

                st = mgr.flushMacEntries(bpath1);
                assertTrue(vlconf.toString(), st.isSuccess());
            }
        }

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * Test method for {@link VTNManagerImpl#receiveDataPacket(RawPacket)}.
     * The case both VlanMap and PortMap applied to vBridge.
     */
    @Test
    public void testReceiveDataPacketBothMapped() {
        VTNManagerImpl mgr = vtnMgr;
        TestStub stub = stubObj;
        ISwitchManager swmgr = mgr.getSwitchManager();
        ITopologyManager topomgr = mgr.getTopologyManager();
        short[] vlans = new short[] { 0, 10, 4095 };
        ConcurrentMap<VBridgePath, Set<NodeConnector>> mappedConnectors
            = new ConcurrentHashMap<VBridgePath, Set<NodeConnector>>();
        Status st = null;

        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname1 = "vbridge1";
        VBridgePath bpath1 = new VBridgePath(tname, bname1);
        String bname2 = "vbridge2";
        VBridgePath bpath2 = new VBridgePath(tname, bname2);
        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        bpathlist.add(bpath1);
        bpathlist.add(bpath2);
        createTenantAndBridge(mgr, tpath, bpathlist);

        mappedConnectors.put(bpath1, new HashSet<NodeConnector>());
        mappedConnectors.put(bpath2, new HashSet<NodeConnector>());

        Set<Node> existNodes = swmgr.getNodes();
        Set<NodeConnector> existConnectors = new HashSet<NodeConnector>();
        for (Node node: existNodes) {
            existConnectors.addAll(swmgr.getNodeConnectors(node));
        }

        Set<Node> vmapNodes = new HashSet<Node>(existNodes);
        vmapNodes.add(null);

        // add interface
        for (short i = 0; i < 4; i++) {
            int inode = 0;
            for (Node node : existNodes) {
                String ifname = "vinterface" + inode + (i + 10);
                VBridgeIfPath ifp = new VBridgeIfPath(tname, (i < 2) ? bname1 : bname2,
                                                    ifname);
                st = mgr.addBridgeInterface(ifp, new VInterfaceConfig(null, true));
                assertTrue(ifp.toString(), st.isSuccess());
                inode++;
            }
        }

        // add a disabled interface
        VBridgeIfPath difp = new VBridgeIfPath(tname, bname1, "vinterface_disabled");
        st = mgr.addBridgeInterface(difp, new VInterfaceConfig(null, false));
        assertTrue(st.toString(), st.isSuccess());

        Node dnode = NodeCreator.createOFNode(Long.valueOf((long)2));
        SwitchPort dport = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW,
                String.valueOf(10));
        st = mgr.setPortMap(difp, new PortMapConfig(dnode, dport, (short)0));
        assertTrue(st.toString(), st.isSuccess());

        // add a nomapped interface
        VBridgeIfPath nomapifp = new VBridgeIfPath(tname, bname1, "vinterface_nomap");
        st = mgr.addBridgeInterface(nomapifp, new VInterfaceConfig(null, false));
        assertTrue(st.toString(), st.isSuccess());


        // null case
//        PacketResult result = mgr.receiveDataPacket(null);
//        assertEquals(PacketResult.IGNORED, result);

        for (short vlan: vlans) {
            // add port mapping
            Set<PortVlan> portMappedThis = new HashSet<PortVlan>();
            Set<PortVlan> portMappedOther = new HashSet<PortVlan>();

            for (short i = 0; i < 4; i++) {
                int inode = 0;
                for (Node node : existNodes) {
                    short setvlan = (i != 1) ? vlan : 100;
                    String ifname = "vinterface" + inode + (i + 10);
                    VBridgeIfPath ifp = new VBridgeIfPath(tname, (i < 2) ? bname1 : bname2,
                                                            ifname);
                    SwitchPort port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW,
                                                    String.valueOf(i + 10));
                    PortMapConfig pmconf = new PortMapConfig(node, port, setvlan);
                    st = mgr.setPortMap(ifp, pmconf);
                    assertTrue(ifp.toString() + "," + pmconf.toString(), st.isSuccess());

                    NodeConnector mapnc = NodeConnectorCreator.createOFNodeConnector(
                                                        Short.valueOf((short)(i + 10)), node);
                    Set<NodeConnector> set  = mappedConnectors.get((i < 2)? bpath1 : bpath2);
                    set.add(mapnc);
                    mappedConnectors.put((i < 2) ? bpath1 : bpath2, set);
                    if (i < 2) {
                        portMappedThis.add(new PortVlan(mapnc, setvlan));
                    } else {
                        portMappedOther.add(new PortVlan(mapnc, setvlan));
                    }

                    inode++;
                }
            }

            for (Node vlanMapNode : vmapNodes) {
                for (short vmapVlan : vlans) {
                    Set<PortVlan> mappedThis = new HashSet<PortVlan>();
                    VlanMapConfig vlconf = new VlanMapConfig(vlanMapNode, vmapVlan);
                    VlanMap map = null;
                    try {
                        map = mgr.addVlanMap(bpath1, vlconf);
                    } catch (VTNException e) {
                        unexpected(e);
                    }

//                    Set<PortVlan> mappedThis = new HashSet<PortVlan>();
                    for (Node node: existNodes) {
                        if (vlanMapNode == null || node.equals(vlanMapNode)) {
                            for (NodeConnector nc : swmgr.getNodeConnectors(node)) {
                                if (!topomgr.isInternal(nc)) {
                                    mappedThis.add(new PortVlan(nc, vmapVlan));
                                }
                            }
                        }
                    }

                    mappedThis.addAll(portMappedThis);
                    mappedThis.removeAll(portMappedOther);

                    testReceiveDataPacketBCLoop(mgr, bpath1, MapType.PORT, mappedThis, stub);

                    st = mgr.flushMacEntries(bpath1);
                    assertTrue("vlan=" + vlan, st.isSuccess());

//                  testReceiveDataPacketUCLoopLearned(mgr, bpath1, MapType.PORT, mappedThis, stub);

//                  st = mgr.flushMacEntries(bpath1);
//               assertTrue("vlan=" + vlan, st.isSuccess());

                    testReceiveDataPacketUCLoop(mgr, bpath1, MapType.PORT, mappedThis, stub);

                    st = mgr.flushMacEntries(bpath1);
                    assertTrue("vlan=" + vlan, st.isSuccess());

                    testReceiveDataPacketUCLoopLearned(mgr, bpath1, MapType.PORT, mappedThis, stub);

                    st = mgr.flushMacEntries(bpath1);
                    assertTrue("vlan=" + vlan, st.isSuccess());

                    st = mgr.removeVlanMap(bpath1, map.getId());
                    assertTrue(vlconf.toString(), st.isSuccess());

                }
            }

         }

        st = mgr.removeBridge(bpath2);
        assertTrue(st.isSuccess());

        // in case received pacekt from no mapped port.
        for (NodeConnector nc: mappedConnectors.get(bpath2)) {
            byte iphost = 1;
            for (EthernetAddress ea: createEthernetAddresses(false)) {
                byte [] bytes = ea.getValue();
                byte [] src = new byte[] {bytes[0], bytes[1], bytes[2],
                                        bytes[3], bytes[4], bytes[5]};
                byte [] dst = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                        (byte)0xff, (byte)0xff, (byte)0xff};
                byte [] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)iphost};
                byte [] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};
                short vlan = -1;
                RawPacket inPkt = createARPRawPacket(src, dst, sender, target, (vlan > 0) ? vlan : -1, nc, ARP.REQUEST);

                PacketResult result = mgr.receiveDataPacket(inPkt);
                if (nc != null) {
                    assertEquals(nc.toString() + "," + ea.toString(), PacketResult.IGNORED, result);
                } else {
                    assertEquals(ea.toString(), PacketResult.IGNORED, result);
                }

                List<RawPacket> transDatas = stub.getTransmittedDataPacket();
                assertEquals(nc.toString() + "," + ea.toString(), 0, transDatas.size());
                iphost++;
            }
        }

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    /**
     * Test method for
     * {@link VTNManagerImpl#find(InetAddress)},
     * {@link internal.VTNManagerImpl#findHost(InetAddress, Set)},
     * {@link VTNManagerImpl#probe(HostNodeConnector)},
     * {@link VTNManagerImpl#probeHost(HostNodeConnector)}.
     */
    @Test
    public void testFindProbe() {
        VTNManagerImpl mgr = vtnMgr;
        TestStub stub = stubObj;
        ISwitchManager swmgr = mgr.getSwitchManager();
        short[] vlans = new short[] { 0, 10, 4095 };
        byte [] mac = new byte [] { 0x00, 0x00, 0x00, 0x11, 0x22, 0x33};
        byte[] cntMac = swmgr.getControllerMAC();
        ConcurrentMap<VBridgePath, Set<NodeConnector>> mappedConnectors
            = new ConcurrentHashMap<VBridgePath, Set<NodeConnector>>();
        Status st = null;

        String tname = "vtn";
        VTenantPath tpath = new VTenantPath(tname);
        String bname1 = "vbridge1";
        VBridgePath bpath1 = new VBridgePath(tname, bname1);
        String bname2 = "vbridge2";
        VBridgePath bpath2 = new VBridgePath(tname, bname2);
        List<VBridgePath> bpathlist = new ArrayList<VBridgePath>();
        bpathlist.add(bpath1);
        bpathlist.add(bpath2);
        createTenantAndBridge(mgr, tpath, bpathlist);

        mappedConnectors.put(bpath1, new HashSet<NodeConnector>());
        mappedConnectors.put(bpath2, new HashSet<NodeConnector>());

        Set<Node> existNodes = swmgr.getNodes();
        Set<NodeConnector> existConnectors = new HashSet<NodeConnector>();
        for (Node node: existNodes) {
            existConnectors.addAll(swmgr.getNodeConnectors(node));
        }

        // add interface
        for (short i = 0; i < 4; i++) {
            int inode = 0;
            for (Node node : existNodes) {
                String ifname = "vinterface" + inode + (i + 10);
                VBridgeIfPath ifp = new VBridgeIfPath(tname,
                        (i < 2) ? bname1 : bname2, ifname);
                st = mgr.addBridgeInterface(ifp, new VInterfaceConfig(null, true));
                assertTrue(ifp.toString(), st.isSuccess());
                inode++;
            }
        }

        InetAddress ia = null;
        try {
            ia = InetAddress.getByAddress(new byte[] {
                    (byte)10, (byte)0, (byte)0, (byte)1});
        } catch (UnknownHostException e) {
            unexpected(e);
        }

        InetAddress ia6 = null;
        try {
            ia6 = InetAddress.getByAddress(new byte[]{
                    (byte)0x20, (byte)0x01, (byte)0x04, (byte)0x20,
                    (byte)0x02, (byte)0x81, (byte)0x10, (byte)0x04,
                    (byte)0x0e1, (byte)0x23, (byte)0xe6, (byte)0x88,
                    (byte)0xd6, (byte)0x55, (byte)0xa1, (byte)0xb0});
        } catch (UnknownHostException e) {
            unexpected(e);
        }

        for (short vlan: vlans) {
            // add port mapping
            for (short i = 0; i < 4; i++) {
                int inode = 0;
                for (Node node : existNodes) {
                    String ifname = "vinterface" + inode + (i + 10);
                    VBridgeIfPath ifp = new VBridgeIfPath(tname, (i < 2) ? bname1 : bname2,
                                                            ifname);
                    SwitchPort port = new SwitchPort(NodeConnector.NodeConnectorIDType.OPENFLOW,
                                                    String.valueOf(i + 10));
                    PortMapConfig pmconf = new PortMapConfig(node, port, vlan);
                    st = mgr.setPortMap(ifp, pmconf);
                    assertTrue(ifp.toString() + "," + pmconf.getVlan(), st.isSuccess());

                    NodeConnector mapnc = NodeConnectorCreator.createOFNodeConnector(
                                                        Short.valueOf((short)(i + 10)), node);
                    Set<NodeConnector> set  = mappedConnectors.get((i < 2)? bpath1 : bpath2);
                    set.add(mapnc);
                    mappedConnectors.put((i < 2)? bpath1 : bpath2, set);
                    inode++;
                }
            }

            Set<Set<VBridgePath>> bpathset = new HashSet<Set<VBridgePath>>();
            bpathset.add(null);
            Set<VBridgePath> bpaths1 = new HashSet<VBridgePath>();
            bpaths1.add(bpath1);
            bpathset.add(bpaths1);
            Set<VBridgePath> bpaths2 = new HashSet<VBridgePath>(bpaths1);
            bpaths2.add(bpath2);
            bpathset.add(bpaths2);

            mgr.findHost(null, bpaths1);
            List<RawPacket> transDatas = stub.getTransmittedDataPacket();
            assertEquals(0, transDatas.size());

            mgr.findHost(ia6, bpaths1);
            transDatas = stub.getTransmittedDataPacket();
            assertEquals(0, transDatas.size());

            for (Set<VBridgePath> bpaths : bpathset) {
                int numMapped = 0;
                Set<PortVlan> mappedThis = new HashSet<PortVlan>();
                if (bpaths == null || bpaths.contains(bpath1)) {
                    for (NodeConnector nc : mappedConnectors.get(bpath1)) {
                        mappedThis.add(new PortVlan(nc, vlan));
                        numMapped++;
                    }
                }
                if (bpaths == null || bpaths.contains(bpath2)) {
                    for (NodeConnector nc : mappedConnectors.get(bpath2)) {
                        mappedThis.add(new PortVlan(nc, vlan));
                        numMapped++;
                    }
                }

                // test find()
                if (bpaths == null) {
                    mgr.find(ia);
                } else {
                    mgr.findHost(ia, bpaths);
                }

                transDatas = stub.getTransmittedDataPacket();
                assertEquals(numMapped, transDatas.size());

                for (RawPacket raw : transDatas) {
                    String emsgr = "vlan=" + vlan + ",(raw)" + raw.toString();
                    Ethernet pkt = (Ethernet)stub.decodeDataPacket(raw);
                    short outVlan;
                    if (pkt.getEtherType() == EtherTypes.VLANTAGGED.shortValue()) {
                        IEEE8021Q vlantag = (IEEE8021Q)pkt.getPayload();
                        assertTrue(emsgr,
                            mappedThis.contains(new PortVlan(raw.getOutgoingNodeConnector(), vlantag.getVid())));
                        outVlan = vlantag.getVid();
                    } else {
                        assertTrue(emsgr,
                                mappedThis.contains(new PortVlan(raw.getOutgoingNodeConnector(), (short)0)));
                        outVlan = (short)0;
                    }

                    PortVlan outPv = new PortVlan(raw.getOutgoingNodeConnector(), outVlan);
                    assertTrue(raw.getOutgoingNodeConnector().toString(),
                        mappedThis.contains(outPv));
//                    assertFalse(raw.getOutgoingNodeConnector().toString(),
//                        noMappedThis.contains(raw.getOutgoingNodeConnector()));

                    Ethernet eth = (Ethernet)pkt;

                    checkOutEthernetPacket("vlan=" + vlan, eth, EtherTypes.ARP,
                            swmgr.getControllerMAC(), new byte[] {-1, -1, -1, -1, -1, -1},
                            vlan, EtherTypes.IPv4, ARP.REQUEST,
                            swmgr.getControllerMAC(), new byte[] {0, 0, 0, 0, 0, 0},
                            new byte[] {0, 0, 0, 0}, ia.getAddress());
                }

                // probe()
                for (PortVlan pv : mappedThis) {
                    HostNodeConnector hnode = null;
                    try {
                        hnode = new HostNodeConnector(mac, ia, pv.getNodeConnector(), pv.getVlan());
                    } catch (ConstructionException e) {
                        unexpected(e);
                    }

                    mgr.probe(hnode);

                    transDatas = stub.getTransmittedDataPacket();
                    assertEquals(pv.toString(), 1, transDatas.size());

                    RawPacket raw = transDatas.get(0);
                    assertEquals(pv.toString(), pv,
                            new PortVlan(raw.getOutgoingNodeConnector(), vlan));

                    Ethernet eth = (Ethernet)stub.decodeDataPacket(raw);
                    checkOutEthernetPacket("vlan=" + vlan, eth, EtherTypes.ARP,
                            swmgr.getControllerMAC(), mac,
                            vlan, EtherTypes.IPv4, ARP.REQUEST,
                            swmgr.getControllerMAC(), mac,
                            new byte[] {0, 0, 0, 0}, ia.getAddress());
                }
            }
        }

        // not OPENFLOW Node
        Node n = null;
        try {
            n = new Node(Node.NodeIDType.ONEPK, "Node ID: 0");
        } catch (ConstructionException e) {
            unexpected(e);
        }
        NodeConnector nc = NodeConnectorCreator.createNodeConnector(NodeConnector.NodeConnectorIDType.ONEPK,
                    "Port: 0", n);
        HostNodeConnector hnode = null;
        try {
            hnode = new HostNodeConnector(mac, ia, nc, (short)0);
        } catch (ConstructionException e) {
            unexpected(e);
        }
        boolean result = mgr.probeHost(hnode);
        assertFalse(result);
        List<RawPacket> transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // internal port
        n = NodeCreator.createOFNode(0L);
        NodeConnector innc = NodeConnectorCreator.createOFNodeConnector(
                Short.valueOf((short)15), n);
        try {
            hnode = new HostNodeConnector(mac, ia, innc, (short)0);
        } catch (ConstructionException e) {
            unexpected(e);
        }
        result = mgr.probeHost(hnode);
        assertFalse(result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // IPv6 host
        innc = NodeConnectorCreator.createOFNodeConnector(
                Short.valueOf((short)10), n);
        try {
            hnode = new HostNodeConnector(mac, ia6, innc, (short)0);
        } catch (ConstructionException e) {
            unexpected(e);
        }
        result = mgr.probeHost(hnode);
        assertFalse(result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // nodeconnector is not mapped
        mgr.removeBridge(bpath2);
        nc = NodeConnectorCreator.createOFNodeConnector(
                Short.valueOf((short)14), n);
        try {
            hnode = new HostNodeConnector(mac, ia, nc, (short)0);
        } catch (ConstructionException e) {
            unexpected(e);
        }
        result = mgr.probeHost(hnode);
        assertFalse(result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());

        // in case if is disabled.
        String ifname = "vinterface010";
        VBridgeIfPath ifp = new VBridgeIfPath(tname, bname1, ifname);
        mgr.modifyBridgeInterface(ifp,
                new VInterfaceConfig(null, Boolean.FALSE), true);

        PortMap map = null;
        try {
            map = mgr.getPortMap(ifp);
        } catch (VTNException e) {
            unexpected(e);
        }

        try {
            hnode = new HostNodeConnector(mac, ia, map.getNodeConnector(),
                    map.getConfig().getVlan());
        } catch (ConstructionException e) {
            unexpected(e);
        }

        result = mgr.probeHost(hnode);
        assertFalse(result);
        transDatas = stub.getTransmittedDataPacket();
        assertEquals(0, transDatas.size());


        // if null case
        mgr.probe(null);

        st = mgr.removeTenant(tpath);
        assertTrue(st.isSuccess());
    }

    // private methods.

    /**
     * common method for test which use BroadCast Packet (ARP Request packet).
     * this method expected that action for received packet is flooding to mapped port.
     */
    private void testReceiveDataPacketBCLoop(VTNManagerImpl mgr, VBridgePath bpath,
           MapType type, Set<PortVlan> mappedThis, TestStub stub) {
        testReceiveDataPacketCommonLoop(mgr, bpath, MapType.PORT, mappedThis, stub, (short)1, EtherTypes.ARP, null);
    }

    /**
     * common method for test which use UniCast Packet (IPv4 packet).
     * this method expected that action for received packet is flooding to mapped port.
     */
    private void testReceiveDataPacketUCLoop(VTNManagerImpl mgr, VBridgePath bpath,
            MapType type, Set<PortVlan> mappedThis, TestStub stub) {
        testReceiveDataPacketCommonLoop(mgr, bpath, MapType.PORT, mappedThis, stub, (short)0, EtherTypes.IPv4, null);
    }

    /**
     * common method for
     * {@link testReceiveDataPacketBCLoop} and {@link testReceiveDataPacketUCLoop}.
     * this method expected that action for received packet is flooding to mapped port.
     */
    private void testReceiveDataPacketCommonLoop(VTNManagerImpl mgr, VBridgePath bpath,
            MapType type, Set<PortVlan> mappedThis, TestStub stub, short bc,
            EtherTypes ethtype, NodeConnector targetnc) {
        ISwitchManager swmgr = mgr.getSwitchManager();
        byte[] cntMac = swmgr.getControllerMAC();

        for (PortVlan inPv: mappedThis) {
            NodeConnector inNc = inPv.getNodeConnector();
            short inVlan = inPv.getVlan();
            if (targetnc != null && !targetnc.equals(inNc)) {
                continue;
            }
            byte iphost = 1;
            for (EthernetAddress ea: createEthernetAddresses(false)) {
                String emsg = "(input portvlan)" + inPv.toString() + ",(input eth)" + ea.toString();
                byte [] bytes = ea.getValue();
                byte [] src = new byte[] {bytes[0], bytes[1], bytes[2],
                                       bytes[3], bytes[4], bytes[5]};
                byte [] dst;
                if (bc > 0) {
                    dst = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                      (byte)0xff, (byte)0xff, (byte)0xff};
                } else {
                    dst = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                        (byte)0xff, (byte)0xff, (byte)0x11};
                }
                byte [] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)iphost};
                byte [] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};

                RawPacket inPkt = null;
                if (ethtype.shortValue() == EtherTypes.IPv4.shortValue()) {
                    inPkt = createIPv4RawPacket(src, dst, sender, target,
                                                    (inVlan > 0) ? inVlan : -1, inNc);
                } else if (ethtype.shortValue() == EtherTypes.ARP.shortValue()){
                    inPkt = createARPRawPacket(src, dst, sender, target,
                            (inVlan > 0) ? inVlan : -1, inNc, ARP.REQUEST);
                }
                Ethernet inPktDecoded = (Ethernet)stub.decodeDataPacket(inPkt);
                PacketResult result = mgr.receiveDataPacket(inPkt);

                if (inNc != null &&
                    inNc.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                    assertEquals(emsg, PacketResult.KEEP_PROCESSING, result);
                    expireFlows(mgr, stub);

                    MacAddressEntry entry = null;
                    try {
                        entry = mgr.getMacEntry(bpath, new EthernetAddress(src));
                    } catch (Exception e) {
                        unexpected(e);
                    }

                    Packet payload = inPktDecoded.getPayload();
                    if (inPktDecoded.getEtherType() == EtherTypes.VLANTAGGED.shortValue()) {
                        payload = payload.getPayload();
                    }
                    if (payload instanceof ARP) {
                        assertTrue(inNc.toString() + ea.toString(),
                                entry.getInetAddresses().size() == 1);
                        assertArrayEquals(emsg, sender, entry.getInetAddresses().iterator().next().getAddress());
                    } else {
                        assertTrue(emsg, entry.getInetAddresses().size() == 0);
                    }

                    List<RawPacket> transDatas = stub.getTransmittedDataPacket();

                    if (ethtype.shortValue() == EtherTypes.IPv4.shortValue()) {
                        assertEquals(emsg, mappedThis.size(), transDatas.size());
                    } else {
                        assertEquals(emsg, mappedThis.size() - 1, transDatas.size());
                    }

                    for (RawPacket raw: transDatas) {
                        Ethernet pkt = (Ethernet)stub.decodeDataPacket(raw);
                        String emsgr = emsg + ",(out packet)" + pkt.toString() + ",(in nc)" + raw.getIncomingNodeConnector()
                                            + ",(out nc)" + raw.getOutgoingNodeConnector();
                        short outVlan;
                        if (pkt.getEtherType() == EtherTypes.VLANTAGGED.shortValue()) {
                            IEEE8021Q vlantag = (IEEE8021Q)pkt.getPayload();
                            assertTrue(emsgr,
                                mappedThis.contains(new PortVlan(raw.getOutgoingNodeConnector(), vlantag.getVid())));
                            outVlan = vlantag.getVid();
                        } else {
                            assertTrue(emsgr,
                                    mappedThis.contains(new PortVlan(raw.getOutgoingNodeConnector(), (short)0)));
                            outVlan = (short)0;
                        }

                        PortVlan outPv = new PortVlan(raw.getOutgoingNodeConnector(), outVlan);
                        if (ethtype.shortValue() == EtherTypes.ARP.shortValue() ||
                            (ethtype.shortValue() == EtherTypes.IPv4.shortValue() &&
                            !outPv.equals(inPv))) {

                            if (ethtype.shortValue() == EtherTypes.IPv4.shortValue()) {
                                checkOutEthernetPacketIPv4(emsgr,
                                        (Ethernet)pkt, EtherTypes.IPv4, src, dst, outVlan);
                            } else if (ethtype.shortValue() == EtherTypes.ARP.shortValue()){
                                checkOutEthernetPacket(emsgr,
                                        (Ethernet)pkt, EtherTypes.ARP, src, dst, outVlan,
                                        EtherTypes.IPv4, ARP.REQUEST, src, dst, sender, target);
                            } else {
                                fail("unexpected packet received.");
                            }
                        } else {
                            checkOutEthernetPacket(emsgr,
                                    (Ethernet)pkt, EtherTypes.ARP, cntMac, src, outVlan,
                                    EtherTypes.IPv4, ARP.REQUEST, cntMac, src,
                                    new byte[] {0, 0, 0, 0}, sender);
                        }
                    }
                } else {
                    if (inNc != null) {
                        assertEquals(inNc.toString() + "," + ea.toString(), PacketResult.IGNORED, result);
                    } else {
                        assertEquals(ea.toString(), PacketResult.IGNORED, result);
                    }
                }
                iphost++;
            }
        }
    }

    /**
     * common method for test which use BroadCast Packet (ARP Request packet).
     * this method expected that action for received packet is flooding to mapped port.
     */
    private void testReceiveDataPacketBCLoopLearned(VTNManagerImpl mgr, VBridgePath bpath,
           MapType type, Set<PortVlan> mappedThis, TestStub stub) {
        testReceiveDataPacketCommonLoopLearned(mgr, bpath, MapType.PORT,
                mappedThis, stub, (short)1, EtherTypes.ARP);
    }

    /**
     * common method for test which use UniCast Packet (IPv4 packet).
     * this method expected that action for received packet is output to learned port.
     */
    private void testReceiveDataPacketUCLoopLearned(VTNManagerImpl mgr, VBridgePath bpath,
            MapType type, Set<PortVlan> mappedThis, TestStub stub) {
        testReceiveDataPacketCommonLoopLearned(mgr, bpath, MapType.PORT,
                                mappedThis, stub, (short)0, EtherTypes.IPv4);
    }

    /**
     * common method for
     * {@link testReceiveDataPacketBCLoop} and {@link testReceiveDataPacketUCLoop}.
     * this method expected that action for received packet is a port host is connected to.
     *
     */
    private void testReceiveDataPacketCommonLoopLearned(VTNManagerImpl mgr, VBridgePath bpath,
            MapType type, Set<PortVlan> mappedThis, TestStub stub, short bc, EtherTypes ethtype) {
        ISwitchManager swmgr = mgr.getSwitchManager();
        byte[] cntMac = swmgr.getControllerMAC();

        for (PortVlan learnedPv: mappedThis) {
            NodeConnector learnedNc = learnedPv.getNodeConnector();
            // first learned hosts to vbridge.
            testReceiveDataPacketCommonLoop(mgr, bpath, MapType.PORT, mappedThis,
                    stub, (short)0, EtherTypes.IPv4, learnedNc);

            // mac addresses have been learned at this point.
            for (PortVlan inPortVlan: mappedThis) {
                NodeConnector inNc = inPortVlan.getNodeConnector();
                short inVlan = inPortVlan.getVlan();
                byte tip = 1;
                boolean first = false;
                for (EthernetAddress ea: createEthernetAddresses(false)) {
                    String emsg = "(learned portvlan)" + learnedPv.toString() + "(input portvlan)"
                                    + inPortVlan.toString() + ",(input eth)" + ea.toString();

                    byte [] bytes = ea.getValue();
                    byte [] src;
                    if (bc > 0) {
                        src = new byte[] {(byte)0xff, (byte)0xff, (byte)0xff,
                                            (byte)0xff, (byte)0xff, (byte)0xff};
                    } else {
                        src = new byte[] {(byte)0x00, (byte)0x00, (byte)0x00,
                                            (byte)0xff, (byte)0xff, (byte)0x11};
                    }
                    byte [] dst = new byte[] {bytes[0], bytes[1], bytes[2],
                                            bytes[3], bytes[4], bytes[5]};
                    byte [] sender = new byte[] {(byte)192, (byte)168, (byte)0, (byte)250};
                    byte [] target = new byte[] {(byte)192, (byte)168, (byte)0, (byte)tip};

                    RawPacket inPkt = null;
                    if (ethtype.shortValue() == EtherTypes.IPv4.shortValue()) {
                        inPkt = createIPv4RawPacket(src, dst, sender, target,
                                                        (inVlan > 0) ? inVlan : -1, inNc);
                    } else if (ethtype.shortValue() == EtherTypes.ARP.shortValue()){
                        // not used case.
                    }

                    Ethernet inPktDecoded = (Ethernet)stub.decodeDataPacket(inPkt);
                    PacketResult result = mgr.receiveDataPacket(inPkt);

                    if (inNc != null &&
                        inNc.getType() == NodeConnector.NodeConnectorIDType.OPENFLOW) {
                        assertEquals(emsg, PacketResult.KEEP_PROCESSING, result);
                        expireFlows(mgr, stub);

                        MacAddressEntry entry = null;
                        try {
                            entry = mgr.getMacEntry(bpath, new EthernetAddress(src));
                        } catch (Exception e) {
                            unexpected(e);
                        }

                        Packet payload = inPktDecoded.getPayload();
                        if (inPktDecoded.getEtherType() == EtherTypes.VLANTAGGED.shortValue()) {
                            payload = payload.getPayload();
                        }
                        if (payload instanceof ARP) {
                            assertTrue(emsg, entry.getInetAddresses().size() == 1);
                            assertArrayEquals(emsg,
                                        sender, entry.getInetAddresses().iterator().next().getAddress() );
                        } else {
                            assertTrue(emsg, entry.getInetAddresses().size() == 0);
                        }

                        List<RawPacket> transDatas = stub.getTransmittedDataPacket();
                        if (!first) {
                            assertEquals(emsg, 2, transDatas.size());
                            first = true;
                        } else {
                            assertEquals(emsg, 1, transDatas.size());
                        }

                        for (RawPacket raw : transDatas) {
                            Packet pkt = stub.decodeDataPacket(raw);
                            Ethernet eth = (Ethernet)pkt;
                            short outEthType;
                            short outVlan;
                            String emsgr = emsg + ",(out packet)" + pkt.toString()
                                        + ",(in nc)" + raw.getIncomingNodeConnector()
                                        + ",(out nc)" + raw.getOutgoingNodeConnector();

                            if (eth.getEtherType() == EtherTypes.VLANTAGGED.shortValue()) {
                                IEEE8021Q vlantag = (IEEE8021Q)eth.getPayload();
                                assertTrue(emsgr, mappedThis.contains(new PortVlan(raw.getOutgoingNodeConnector(),
                                            vlantag.getVid())));
                                outEthType = vlantag.getEtherType();
                                outVlan = vlantag.getVid();
                            } else {
                                outEthType = eth.getEtherType();
                                outVlan = (short)-1;
                            }

                            if (outEthType == EtherTypes.ARP.shortValue() &&
                                    raw.getOutgoingNodeConnector().equals(inNc)) {
                                // this is a packet to detect IP address of host
                                // which has no IP address.
                                checkOutEthernetPacket(emsgr,
                                    (Ethernet)pkt, EtherTypes.ARP, cntMac, src, outVlan,
                                    EtherTypes.IPv4, ARP.REQUEST, cntMac, src,
                                    new byte[] {0, 0, 0, 0}, sender);
                            } else {
                                checkOutEthernetPacketIPv4(emsgr,
                                    (Ethernet)pkt, EtherTypes.IPv4, src, dst, outVlan);
                            }
                        }
                    } else {
                        if (inNc != null) {
                            assertEquals(emsg, PacketResult.IGNORED, result);
                        } else {
                            assertEquals(emsg, PacketResult.IGNORED, result);
                        }
                    }
                    tip++;
                }
            }

            Status st = mgr.flushMacEntries(bpath);
            assertTrue(st.isSuccess());
        }
    }

    /**
     * Expire flow entries.
     *
     * @param mgr    VTN Manager service.
     * @param stub   Stub for OSGi service.
     */
    private void expireFlows(VTNManagerImpl mgr, TestStub stub) {
        // Wait for all flow modifications to complete.
        NopFlowTask task = new NopFlowTask(mgr);
        mgr.postFlowTask(task);
        assertSame(FlowModResult.SUCCEEDED, task.getResult(3000));

        Set<FlowEntry> flows = stub.getFlowEntries();
        if (!flows.isEmpty()) {
            for (FlowEntry fent: flows) {
                String flowName = fent.getFlowName();
                if (flowName.endsWith("-0")) {
                    Status status = stub.uninstallFlowEntry(fent);
                    assertTrue(status.isSuccess());
                    mgr.flowRemoved(fent.getNode(), fent.getFlow());
                }
            }

            // Wait for all flow entries to be removed.
            task = new NopFlowTask(mgr);
            mgr.postFlowTask(task);
            assertSame(FlowModResult.SUCCEEDED, task.getResult(3000));
            assertSame(0, stub.getFlowEntries().size());
        }
    }

    /**
     * A dummy flow task to flush pending tasks.
     */
    private class NopFlowTask extends FlowModTask {
        /**
         * Construct a new task.
         *
         * @param mgr  VTN Manager service.
         */
        private NopFlowTask(VTNManagerImpl mgr) {
            super(mgr);
        }

        /**
         * Execute this task.
         *
         * @return  {@code true} is always returned.
         */
        @Override
        protected boolean execute() {
            return true;
        }

        /**
         * Return a logger object for this class.
         *
         * @return  A logger object.
         */
        @Override
        protected Logger getLogger() {
            return LoggerFactory.getLogger(getClass());
        }
    }
}
