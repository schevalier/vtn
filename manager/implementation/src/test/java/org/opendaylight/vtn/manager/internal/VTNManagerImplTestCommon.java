/*
 * Copyright (c) 2013 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.manager.internal;

import static org.junit.Assert.*;

import java.io.File;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Set;


import org.apache.felix.dm.impl.ComponentImpl;
import org.junit.After;
import org.junit.Before;


import org.opendaylight.controller.hosttracker.IfHostListener;
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
import org.opendaylight.controller.sal.topology.TopoEdgeUpdate;
import org.opendaylight.controller.sal.utils.EtherTypes;
import org.opendaylight.controller.sal.utils.GlobalConstants;
import org.opendaylight.controller.sal.utils.Status;
import org.opendaylight.controller.topologymanager.ITopologyManager;
import org.opendaylight.vtn.manager.IVTNManager;
import org.opendaylight.vtn.manager.IVTNManagerAware;
import org.opendaylight.vtn.manager.IVTNModeListener;
import org.opendaylight.vtn.manager.PortMap;
import org.opendaylight.vtn.manager.PortMapConfig;
import org.opendaylight.vtn.manager.VBridge;
import org.opendaylight.vtn.manager.VBridgeConfig;
import org.opendaylight.vtn.manager.VBridgeIfPath;
import org.opendaylight.vtn.manager.VBridgePath;
import org.opendaylight.vtn.manager.VInterface;
import org.opendaylight.vtn.manager.VInterfaceConfig;
import org.opendaylight.vtn.manager.VNodeState;
import org.opendaylight.vtn.manager.VTenant;
import org.opendaylight.vtn.manager.VTenantConfig;
import org.opendaylight.vtn.manager.VTenantPath;
import org.opendaylight.vtn.manager.VlanMap;

/**
 * Common class for {@link VTNManagerImplTest} and {@link VTNManagerImplWithNodesTest}.
 *
 */
public class VTNManagerImplTestCommon extends TestBase {
    protected VTNManagerImpl vtnMgr = null;
    protected TestStub stubObj = null;
    protected static int stubMode = 0;

    class HostListener implements IfHostListener {
        private int hostListenerCalled = 0;

        @Override
        public void hostListener(HostNodeConnector host) {
            hostListenerCalled++;
        }

        int getHostListenerCalled () {
            int ret = hostListenerCalled;
            hostListenerCalled = 0;
            return  ret;
        }
    }

    class VTNModeListenerStub implements IVTNModeListener {
        private int calledCount = 0;
        private Boolean oldactive = null;
        private final long sleepMilliTime = 10L;

        @Override
        public void vtnModeChanged(boolean active) {
            calledCount++;
            oldactive = Boolean.valueOf(active);
        }

        public int getCalledCount() {
            sleep(sleepMilliTime);
            int ret = calledCount;
            calledCount = 0;
            return ret;
        }

        public Boolean getCalledArg() {
            sleep(sleepMilliTime);
            Boolean ret = oldactive;
            oldactive = null;
            return ret;
        }

        void checkCalledInfo(int expCount, Boolean expMode) {
            if (expCount >= 0) {
                assertEquals(expCount, this.getCalledCount());
            }
            assertEquals(expMode.booleanValue(), this.getCalledArg());
        }

        void checkCalledInfo(int expCount) {
            if (expCount >= 0) {
                assertEquals(expCount, this.getCalledCount());
            }
        }
    }

    class VTNManagerAwareData<T, S> {
        T path = null;
        S obj = null;
        UpdateType type = null;
        int count = 0;

        VTNManagerAwareData(T p, S o, UpdateType t, int c) {
            path = p;
            obj = o;
            type = t;
            count = c;
        }
    };

    class VTNManagerAwareStub implements IVTNManagerAware {
        private final long sleepMilliTime = 10L;

        private int vtnChangedCalled = 0;
        private int vbrChangedCalled = 0;
        private int vIfChangedCalled = 0;
        private int vlanMapChangedCalled = 0;
        private int portMapChangedCalled = 0;
        VTNManagerAwareData<VTenantPath, VTenant> vtnChangedInfo = null;
        VTNManagerAwareData<VBridgePath, VBridge> vbrChangedInfo = null;
        VTNManagerAwareData<VBridgeIfPath, VInterface> vIfChangedInfo = null;
        VTNManagerAwareData<VBridgePath, VlanMap> vlanMapChangedInfo = null;
        VTNManagerAwareData<VBridgeIfPath, PortMap> portMapChangedInfo = null;

        @Override
        public void vtnChanged(VTenantPath path, VTenant vtenant, UpdateType type) {
            vtnChangedCalled++;
            vtnChangedInfo = new VTNManagerAwareData<VTenantPath, VTenant>(path, vtenant,
                    type, vtnChangedCalled);
        }

        @Override
        public void vBridgeChanged(VBridgePath path, VBridge vbridge, UpdateType type) {
            vbrChangedCalled++;
            vbrChangedInfo = new VTNManagerAwareData<VBridgePath, VBridge>(path, vbridge, type,
                    vbrChangedCalled);
        }

        @Override
        public void vBridgeInterfaceChanged(VBridgeIfPath path, VInterface viface, UpdateType type) {
            vIfChangedCalled++;
            vIfChangedInfo = new VTNManagerAwareData<VBridgeIfPath, VInterface>(path, viface, type,
                    vIfChangedCalled);
        }

        @Override
        public void vlanMapChanged(VBridgePath path, VlanMap vlmap, UpdateType type) {
            vlanMapChangedCalled++;
            vlanMapChangedInfo = new VTNManagerAwareData<VBridgePath, VlanMap>(path, vlmap, type,
                    vlanMapChangedCalled);
        }

        @Override
        public void portMapChanged(VBridgeIfPath path, PortMap pmap, UpdateType type) {
            portMapChangedCalled++;
            portMapChangedInfo = new VTNManagerAwareData<VBridgeIfPath, PortMap>(path, pmap, type,
                    portMapChangedCalled);
        }

        void checkVtnInfo (int count, VTenantPath path, String name, UpdateType type) {
            sleep(sleepMilliTime);
            assertEquals(count, vtnChangedCalled);
            if (path != null) {
                assertEquals(path, vtnChangedInfo.path);
            }
            if (name != null) {
                assertEquals(name, vtnChangedInfo.obj.getName());
            }
            if (type != null) {
                assertEquals(type, vtnChangedInfo.type);
            }
            vtnChangedCalled = 0;
            vtnChangedInfo = null;
        }

        void checkVbrInfo (int count, VBridgePath path, String name, UpdateType type) {
            sleep(sleepMilliTime);
            assertEquals(count, vbrChangedCalled);
            if (path != null) {
                assertEquals(path, vbrChangedInfo.path);
            }
            if (name != null) {
                assertEquals(name, vbrChangedInfo.obj.getName());
            }
            if (type != null) {
                assertEquals(type, vbrChangedInfo.type);
            }
            vbrChangedCalled = 0;
            vbrChangedInfo = null;
        }

        void checkVIfInfo (int count, VBridgeIfPath path, String name, UpdateType type) {
            sleep(sleepMilliTime);
            assertEquals(count, vIfChangedCalled);
            if (path != null) {
                assertEquals(path, vIfChangedInfo.path);
            }
            if (name != null) {
                assertEquals(name, vIfChangedInfo.obj.getName());
            }
            if (type != null) {
                assertEquals(type, vIfChangedInfo.type);
            }
            vIfChangedCalled = 0;
            vIfChangedInfo = null;
        }

        void checkVlmapInfo (int count, VBridgePath path, String id, UpdateType type) {
            sleep(sleepMilliTime);
            assertEquals(count, vlanMapChangedCalled);
            if (path != null) {
                assertEquals(path, vlanMapChangedInfo.path);
            }
            if (id != null) {
                assertEquals(id, vlanMapChangedInfo.obj.getId());
            }
            if (type != null) {
                assertEquals(type, vlanMapChangedInfo.type);
            }
            vlanMapChangedCalled = 0;
            vlanMapChangedInfo = null;
        }

        void checkPmapInfo (int count, VBridgeIfPath path, PortMapConfig pconf, UpdateType type) {
            sleep(sleepMilliTime);
            assertEquals(count, portMapChangedCalled);
            if (path != null) {
                assertEquals(path, portMapChangedInfo.path);
            }
            if (pconf != null) {
                assertEquals(pconf, portMapChangedInfo.obj.getConfig());
            }
            if (type != null) {
                assertEquals(type, portMapChangedInfo.type);
            }
            portMapChangedCalled = 0;
            portMapChangedInfo = null;
        }

        void checkAllNull() {
            sleep(sleepMilliTime);
            assertEquals(0, vtnChangedCalled);
            assertNull(vtnChangedInfo);
            assertEquals(0, vbrChangedCalled);
            assertNull(vbrChangedInfo);
            assertEquals(0, vIfChangedCalled);
            assertNull(vIfChangedInfo);
            assertEquals(0, vlanMapChangedCalled);
            assertNull(vlanMapChangedInfo);
            assertEquals(0, portMapChangedCalled);
            assertNull(portMapChangedInfo);
        }
    };


    @Before
    public void before() {
        File confdir = new File(GlobalConstants.STARTUPHOME.toString());
        boolean result = confdir.exists();
        if (!result) {
            result = confdir.mkdirs();
        } else {
            File[] list = confdir.listFiles();
            for (File f : list) {
                f.delete();
            }
        }

        vtnMgr = new VTNManagerImpl();
        ComponentImpl c = new ComponentImpl(null, null, null);
        GlobalResourceManager grsc = new GlobalResourceManager();
        stubObj = new TestStub(stubMode);

        Hashtable<String, String> properties = new Hashtable<String, String>();
        properties.put("containerName", "default");
        c.setServiceProperties(properties);

        grsc.setClusterGlobalService(stubObj);
        grsc.init(c);
        vtnMgr.setResourceManager(grsc);
        vtnMgr.setClusterContainerService(stubObj);
        vtnMgr.setSwitchManager(stubObj);
        vtnMgr.setTopologyManager(stubObj);
        vtnMgr.setDataPacketService(stubObj);
        vtnMgr.setRouting(stubObj);
        vtnMgr.setHostTracker(stubObj);
        vtnMgr.init(c);
    }

    @After
    public void after() {

        vtnMgr.destroy();

        String currdir = new File(".").getAbsoluteFile().getParent();
        File confdir = new File(GlobalConstants.STARTUPHOME.toString());

        if (confdir.exists()) {
            File[] list = confdir.listFiles();
            for (File f : list) {
                f.delete();
            }

            while (confdir != null && confdir.getAbsolutePath() != currdir) {
                confdir.delete();
                String pname = confdir.getParent();
                if (pname == null) {
                    break;
                }
                confdir = new File(pname);
            }
        }
    }


    /**
     * method for setup enviroment.
     * create 1 Tenant and bridges
     */
    protected void createTenantAndBridge(IVTNManager mgr, VTenantPath tpath,
            List<VBridgePath> bpaths) {

        Status st = mgr.addTenant(tpath, new VTenantConfig(null));
        assertTrue(st.isSuccess());
        assertTrue(mgr.isActive());

        for (VBridgePath bpath : bpaths) {
            st = mgr.addBridge(bpath, new VBridgeConfig(null));
            assertTrue(st.isSuccess());
        }
    }

    /**
     * method for setup enviroment.
     * create 1 Tenant and bridges and vinterfaces
     */
    protected void createTenantAndBridgeAndInterface(IVTNManager mgr, VTenantPath tpath,
            List<VBridgePath> bpaths, List<VBridgeIfPath> ifpaths) {

        Status st = mgr.addTenant(tpath, new VTenantConfig(null));
        assertTrue(st.isSuccess());
        assertTrue(mgr.isActive());

        for (VBridgePath bpath : bpaths) {
            st = mgr.addBridge(bpath, new VBridgeConfig(null));
            assertTrue(st.isSuccess());
        }

        for (VBridgeIfPath ifpath : ifpaths) {
            VInterfaceConfig ifconf = new VInterfaceConfig(null, null);
            st = mgr.addBridgeInterface(ifpath, ifconf);
            assertTrue(st.isSuccess());
        }
    }

    /**
     * check a Ethernet packet whether setted expected parametor in the packet.
     * (for IPv4 packet)
     *
     * @param msg   if check is failed, report error with a message specified this.
     * @param eth   input ethernet frame data.
     * @param ethType   expected ethernet type.
     * @param destMac   expected destination mac address.
     * @param srcMac    expected source mac address.
     * @param vlan  expected vlan id. (if expected untagged, specify 0 or less than 0)
     */
    protected void checkOutEthernetPacketIPv4 (String msg, Ethernet eth, EtherTypes ethType,
            byte[] srcMac, byte[] destMac,  short vlan) {

        checkOutEthernetPacket(msg, eth, ethType, srcMac, destMac, vlan, null, (short)-1,
                null, null, null, null);
    }
}