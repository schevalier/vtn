/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.javaapi.openstack.dbmanager;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Vector;

import org.opendaylight.vtn.core.util.Logger;
import org.opendaylight.vtn.javaapi.constants.VtnServiceConsts;
import org.opendaylight.vtn.javaapi.exception.VtnServiceException;
import org.opendaylight.vtn.javaapi.exception.VtnServiceExceptionHandler;
import org.opendaylight.vtn.javaapi.ipc.enums.UncJavaAPIErrorCode;

/**
 * DataBase Connection Pool Class. Implements the connection pooling mechanism
 * for OpenStack operations
 */
public class DataBaseConnectionPool implements Runnable {

	private static final Logger LOG = Logger
			.getLogger(DataBaseConnectionPool.class.getName());

	private final ConnectionProperties connectionProperties;

	private boolean pendingConnection = false;

	private Vector<Connection> availableConnections, usedConnections;

	/**
	 * Constructor of Database Connection Pooling. Initialize the connection
	 * pool with initial connection pool size
	 * 
	 * @param connectionProperties
	 * @param exceptionHandler
	 * @throws VtnServiceException
	 */
	public DataBaseConnectionPool(ConnectionProperties connectionProperties,
			final VtnServiceExceptionHandler exceptionHandler)
			throws VtnServiceException {

		LOG.trace("Start DataBaseConnectionPool()");

		this.connectionProperties = connectionProperties;

		if (this.connectionProperties.getInitialConnections() > this.connectionProperties
				.getMaxPossibleConnections()) {
			this.connectionProperties
					.setInitialConnections(this.connectionProperties
							.getMaxPossibleConnections());
		}

		this.availableConnections = new Vector<Connection>(
				connectionProperties.getInitialConnections());
		this.usedConnections = new Vector<Connection>();

		try {
			// create connection with initial connection pool size
			for (int i = 0; i < this.connectionProperties
					.getInitialConnections(); i++) {
				this.availableConnections.addElement(createConnection());
			}
		} catch (final SQLException e) {
			LOG.error("Connection Pooling Initialization Error.");
			exceptionHandler
					.raise(Thread.currentThread().getStackTrace()[1]
							.getClassName()
							+ VtnServiceConsts.HYPHEN
							+ Thread.currentThread().getStackTrace()[1]
									.getMethodName(),
							UncJavaAPIErrorCode.DB_CONN_INIT_ERROR
									.getErrorCode(),
							UncJavaAPIErrorCode.DB_CONN_INIT_ERROR
									.getErrorMessage(), e);
		}

		LOG.debug("Connection Pool Initialized with "
				+ this.connectionProperties.getInitialConnections());

		LOG.trace("Complete DataBaseConnectionPool()");
	}

	/**
	 * Returns the instance of connection for connection pool. If connection is
	 * not available in connection pool then create new connection till max
	 * connection pool size
	 * 
	 * @return - Connection instance
	 * @throws SQLException
	 */
	public synchronized Connection getConnection() throws SQLException {

		LOG.trace("Start DataBaseConnectionPool#getConnection()");

		if (!availableConnections.isEmpty()) {
			/*
			 * if connection is available in prepared connection pool, the
			 * return from connection pool. Update the available connection and
			 * used connection list as per result of operation
			 */
			LOG.debug("Connection can be provided by initialized connection pool.");
			final Connection connection = availableConnections.lastElement();
			availableConnections
					.removeElementAt(availableConnections.size() - 1);

			if (connection.isClosed()) {
				LOG.warning("Connection had been closed. Create new connection and return");
				notifyAll();
				return getConnection();
			} else {
				LOG.debug("Use connection : " + connection);
				usedConnections.addElement(connection);
				LOG.trace("Complete DataBaseConnectionPool#getConnection()");
				return connection;
			}
		} else {
			LOG.debug("Connection cannot be provided by initialized connection pool.");
			/*
			 * If connection is not available then create new connection after
			 * checking max connection pool size
			 */
			if ((countConnections() < connectionProperties
					.getMaxPossibleConnections()) && !pendingConnection) {
				LOG.debug("Initial connections are in use, create new back ground connection.");
				createBackGroundConnection();
			} else if (!connectionProperties.isWaitforUsedConnections()) {
				/*
				 * if waiting is allowed in system, the throw error in case of
				 * connection pool size is already reached to its maximum value
				 */
				LOG.error("Waiting is not required, throw error.");
				throw new SQLException("Connection limit reached");
			}
			try {
				// wait for connections to be free, if wait is allowed
				LOG.debug("Wait is possible, so wait till other connections is freed.");
				wait();
			} catch (final InterruptedException ie) {
				LOG.debug("Wait is interuppted by some other thread.");
			}
			return getConnection();
		}
	}

	/**
	 * Create new connection as new thread
	 */
	private void createBackGroundConnection() {

		LOG.trace("Start DataBaseConnectionPool#createBackGroundConnection()");

		LOG.debug("Back ground connection required, means some connection is in pending state.");
		pendingConnection = true;
		try {
			LOG.debug("Create connection and return.");
			final Thread connectThread = new Thread(this);
			connectThread.start();
		} catch (final OutOfMemoryError oome) {
			LOG.fatal("Out of memory space error : " + oome);
		}

		LOG.trace("Complete DataBaseConnectionPool#createBackGroundConnection()");
	}

	/**
	 * Start thread to create connection, add newly created connection to
	 * available connection list
	 */
	@Override
	public void run() {

		LOG.trace("Start DataBaseConnectionPool#run()");

		try {
			final Connection connection = createConnection();
			synchronized (this) {
				availableConnections.addElement(connection);
				pendingConnection = false;
				notifyAll();
			}
		} catch (final Exception e) {
			LOG.error("Error ocurred while creating new connection.");
		}

		LOG.trace("Complete DataBaseConnectionPool#run()");
	}

	/**
	 * Create connection by using JDBC APIs
	 * 
	 * @return
	 * @throws SQLException
	 */
	private Connection createConnection() throws SQLException {

		LOG.trace("Start DataBaseConnectionPool#createConnection()");

		try {
			Class.forName(connectionProperties.getDbDriver());
			final Connection connection = DriverManager.getConnection(
					connectionProperties.getDbURL(),
					connectionProperties.getDbUsername(),
					connectionProperties.getDbPassword());
			connection.setAutoCommit(false);
			LOG.trace("Complete DataBaseConnectionPool#createConnection()");
			return connection;
		} catch (final ClassNotFoundException cnfe) {
			LOG.error("Can't find class for driver : "
					+ connectionProperties.getDbDriver());
			throw new SQLException("Can't find class for driver: "
					+ connectionProperties.getDbDriver());
		}
	}

	/**
	 * Free specify connection to make it available for new requests
	 * 
	 * @param connection
	 */
	public synchronized void freeConnection(Connection connection) {

		LOG.trace("Start DataBaseConnectionPool#freeConnection()");

		usedConnections.removeElement(connection);
		availableConnections.addElement(connection);
		notifyAll();

		LOG.trace("Complete DataBaseConnectionPool#freeConnection()");
	}

	/**
	 * Get the count of connection available to be used
	 * 
	 * @return
	 */
	public synchronized int countConnections() {

		LOG.trace("Return from DataBaseConnectionPool#countConnections()");

		return availableConnections.size() + usedConnections.size();
	}

	/**
	 * Close all connections and re-initialize available and used connection
	 * lists with no connections
	 */
	public synchronized void closeAllConnections() {

		LOG.trace("Start DataBaseConnectionPool#closeAllConnections()");

		closeConnections(availableConnections);
		availableConnections = new Vector<Connection>();
		closeConnections(usedConnections);
		usedConnections = new Vector<Connection>();

		LOG.trace("Complete DataBaseConnectionPool#closeAllConnections()");
	}

	/**
	 * Close all JDBC connection specified by vector
	 * 
	 * @param connections
	 */
	private void closeConnections(Vector<Connection> connections) {

		LOG.trace("Start DataBaseConnectionPool#closeConnections()");

		try {
			for (int i = 0; i < connections.size(); i++) {

				final Connection connection = connections.elementAt(i);
				if (!connection.isClosed()) {
					LOG.debug("Close connections, if it is not closed.");
					connection.close();
				}
			}
		} catch (final SQLException sqle) {
			LOG.error("Error occurred while closing the connection : " + sqle);
		}

		LOG.trace("Complete DataBaseConnectionPool#closeConnections()");
	}
}
