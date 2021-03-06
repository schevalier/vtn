/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.javaapi.openstack.validation;

import java.math.BigInteger;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import org.opendaylight.vtn.core.util.Logger;
import org.opendaylight.vtn.javaapi.constants.VtnServiceConsts;
import org.opendaylight.vtn.javaapi.constants.VtnServiceJsonConsts;
import org.opendaylight.vtn.javaapi.exception.VtnServiceException;
import org.opendaylight.vtn.javaapi.ipc.enums.UncCommonEnum;
import org.opendaylight.vtn.javaapi.ipc.enums.UncJavaAPIErrorCode;
import org.opendaylight.vtn.javaapi.openstack.constants.VtnServiceOpenStackConsts;
import org.opendaylight.vtn.javaapi.resources.AbstractResource;
import org.opendaylight.vtn.javaapi.resources.openstack.PortResource;
import org.opendaylight.vtn.javaapi.resources.openstack.PortsResource;
import org.opendaylight.vtn.javaapi.validation.CommonValidator;
import org.opendaylight.vtn.javaapi.validation.VtnServiceValidator;

/**
 * Port Validation Resource class. Contains methods to validate URI, parameters
 * of POST request body
 * 
 */
public class PortResourceValidator extends VtnServiceValidator {

	/* Logger instance */
	private static final Logger LOG = Logger
			.getLogger(PortResourceValidator.class.getName());

	/* AbstractResource reference pointing to actual Resource class */
	private final AbstractResource resource;

	/* instance for common validation operation */
	private final CommonValidator validator = new CommonValidator();

	/**
	 * Constructor that provide reference of actual Resource class to instance
	 * variable resource
	 * 
	 * @param resource
	 *            - Resource class reference
	 */
	public PortResourceValidator(final AbstractResource resource) {
		this.resource = resource;
	}

	/*
	 * Calls the respective validation method according to type of method
	 * 
	 * @see
	 * org.opendaylight.vtn.javaapi.validation.VtnServiceValidator#validate
	 * (java.lang.String, com.google.gson.JsonObject)
	 */
	@Override
	public void validate(String method, JsonObject requestBody)
			throws VtnServiceException {
		LOG.trace("Start PortResourceValidator#validate()");
		boolean isValid = false;
		try {
			isValid = validateUri();
			if (isValid && requestBody != null
					&& VtnServiceConsts.POST.equalsIgnoreCase(method)) {
				isValid = validatePost(requestBody);
			} else if (isValid) {
				setInvalidParameter(UncCommonEnum.UncResultCode.UNC_METHOD_NOT_ALLOWED
						.getMessage());
				isValid = false;
			}
		} catch (final NumberFormatException e) {
			LOG.error("Invalid value : " + e.getMessage());
			isValid = false;
		} catch (final ClassCastException e) {
			LOG.error("Invalid type : " + e.getMessage());
			isValid = false;
		}

		/*
		 * throw exception in case of validation fail
		 */
		if (!isValid) {
			LOG.error("Validation failure");
			throw new VtnServiceException(Thread.currentThread()
					.getStackTrace()[1].getMethodName(),
					UncJavaAPIErrorCode.VALIDATION_ERROR.getErrorCode(),
					UncJavaAPIErrorCode.VALIDATION_ERROR.getErrorMessage());
		}
		LOG.trace("Complete PortResourceValidator#validate()");
	}

	/**
	 * Validate resource specific URI parameters
	 * 
	 * @see org.opendaylight.vtn.javaapi.validation.VtnServiceValidator#validateUri
	 *      ()
	 */
	@Override
	public boolean validateUri() throws VtnServiceException {
		LOG.trace("Start PortResourceValidator#validateUri()");
		boolean isValid = true;
		if (resource instanceof PortResource) {
			// validation of tenant_id
			final String tenantId = ((PortResource) resource).getTenantId();
			if (tenantId == null
					|| tenantId.isEmpty()
					|| !validator.isValidMaxLengthAlphaNum(tenantId,
							VtnServiceJsonConsts.LEN_31)) {
				isValid = false;
				setInvalidParameter(VtnServiceOpenStackConsts.TENANT_ID
						+ VtnServiceConsts.COLON + tenantId);
			}

			// validation of net_id
			if (isValid) {
				final String netId = ((PortResource) resource).getNetId();
				if (netId == null
						|| netId.isEmpty()
						|| !validator.isValidMaxLengthAlphaNum(netId,
								VtnServiceJsonConsts.LEN_31)) {
					isValid = false;
					setInvalidParameter(VtnServiceOpenStackConsts.NET_ID
							+ VtnServiceConsts.COLON + netId);
				}
			}

			// validation of port_id
			if (isValid) {
				final String portId = ((PortResource) resource).getPortId();
				if (portId == null
						|| portId.isEmpty()
						|| !validator.isValidMaxLengthAlphaNum(portId,
								VtnServiceJsonConsts.LEN_24)) {
					isValid = false;
					setInvalidParameter(VtnServiceOpenStackConsts.PORT_ID
							+ VtnServiceConsts.COLON + portId);
				}
			}
		} else if (resource instanceof PortsResource) {
			// validation of tenant_id
			final String tenantId = ((PortsResource) resource).getTenantId();
			if (tenantId == null
					|| tenantId.isEmpty()
					|| !validator.isValidMaxLengthAlphaNum(tenantId,
							VtnServiceJsonConsts.LEN_31)) {
				isValid = false;
				setInvalidParameter(VtnServiceOpenStackConsts.TENANT_ID
						+ VtnServiceConsts.COLON + tenantId);
			}

			// validation of net_id
			if (isValid) {
				final String netId = ((PortsResource) resource).getNetId();
				if (netId == null
						|| netId.isEmpty()
						|| !validator.isValidMaxLengthAlphaNum(netId,
								VtnServiceJsonConsts.LEN_31)) {
					isValid = false;
					setInvalidParameter(VtnServiceOpenStackConsts.NET_ID
							+ VtnServiceConsts.COLON + netId);
				}
			}
		}
		LOG.trace("Complete PortResourceValidator#validateUri()");
		return isValid;
	}

	/**
	 * Validates the parameters of POST request body
	 * 
	 * @param requestBody
	 *            - JSON request body corresponding to POST operation
	 * @return
	 */
	public boolean validatePost(final JsonObject requestBody) {
		LOG.trace("Start PortResourceValidator#validatePost()");
		boolean isValid = true;
		// validation of mandatory parameters
		if (requestBody == null
				|| !requestBody.has(VtnServiceOpenStackConsts.PORT)
				|| !requestBody.has(VtnServiceOpenStackConsts.DATAPATH_ID)
				|| !requestBody.has(VtnServiceOpenStackConsts.VID)) {
			isValid = false;
			setInvalidParameter(UncCommonEnum.UncResultCode.UNC_INVALID_FORMAT
					.getMessage());
		} else {
			// validation of id
			if (requestBody.has(VtnServiceOpenStackConsts.ID)) {
				final JsonElement id = requestBody
						.get(VtnServiceOpenStackConsts.ID);
				if (id.isJsonNull()
						|| id.getAsString().isEmpty()
						|| !validator.isValidMaxLengthAlphaNum(
								id.getAsString(), VtnServiceJsonConsts.LEN_24)) {
					isValid = false;
					setInvalidParameter(VtnServiceOpenStackConsts.ID
							+ VtnServiceConsts.COLON
							+ (id.isJsonNull() ? id : id.getAsString()));
				}
			}

			// validation of datapath_id
			if (isValid) {
				final JsonElement datapathId = requestBody
						.get(VtnServiceOpenStackConsts.DATAPATH_ID);
				setInvalidParameter(VtnServiceOpenStackConsts.DATAPATH_ID);
				if (datapathId.isJsonNull()
						|| !isValidDataPathId(datapathId.getAsString())) {
					isValid = false;
					setInvalidParameter(VtnServiceOpenStackConsts.DATAPATH_ID
							+ VtnServiceConsts.COLON
							+ (datapathId.isJsonNull() ? datapathId
									: datapathId.getAsString()));
				}
			}

			// validation of port
			if (isValid) {
				final JsonElement port = requestBody
						.get(VtnServiceOpenStackConsts.PORT);
				if (!port.isJsonNull()
						&& !port.getAsString().equalsIgnoreCase(
								VtnServiceOpenStackConsts.NULL)) {
					if (port.getAsString().isEmpty()) {
						isValid = false;
					} else {
						try {
							isValid = validator.isValidBigIntegerRangeString(
									new BigInteger(port.getAsString()),
									VtnServiceJsonConsts.BIG_VAL0,
									VtnServiceJsonConsts.BIG_VAL_4294967040);
						} catch (Exception e) {
							isValid = false;
						}
					}

					// set message as per above checks
					if (!isValid) {
						setInvalidParameter(VtnServiceOpenStackConsts.PORT
								+ VtnServiceConsts.COLON
								+ (port.isJsonNull() ? port : port
										.getAsString()));
					}
				}
			}

			// validation of vid
			if (isValid) {
				final JsonElement vid = requestBody
						.get(VtnServiceOpenStackConsts.VID);
				setInvalidParameter(VtnServiceOpenStackConsts.VID);
				if (vid.isJsonNull() || vid.getAsString().isEmpty()) {
					isValid = false;
				} else {
					try {
						isValid = validator.isValidRange(vid.getAsString(),
								VtnServiceJsonConsts.VAL_1,
								VtnServiceJsonConsts.VAL_4095);
						if (Integer.parseInt(vid.getAsString()) == VtnServiceJsonConsts.VAL_65535) {
							isValid = true;
						}
					} catch (Exception e) {
						isValid = false;
					}
				}

				// set message as per above checks
				if (!isValid) {
					setInvalidParameter(VtnServiceOpenStackConsts.VID
							+ VtnServiceConsts.COLON
							+ (vid.isJsonNull() ? vid : vid.getAsString()));
				}
			}
		}

		// validation of port and datapath_id combination
		if (isValid) {
			final JsonElement port = requestBody
					.get(VtnServiceOpenStackConsts.PORT);
			final JsonElement datapathid = requestBody
					.get(VtnServiceOpenStackConsts.DATAPATH_ID);
			if (datapathid.getAsString().isEmpty()
					&& (!port.isJsonNull() && !port.getAsString()
							.equalsIgnoreCase(VtnServiceOpenStackConsts.NULL))) {
				isValid = false;
				setInvalidParameter("port specified, but datapath_id not specified");
			}
		}

		LOG.trace("Complete PortResourceValidator#validatePost()");
		return isValid;
	}

	/**
	 * Check the validity of datapath_id parameter
	 * 
	 * @param datapathId
	 *            - string representing datapath_id
	 * @return - result as true or false
	 */
	private boolean isValidDataPathId(String datapathId) {
		boolean isValid = true;
		if (!datapathId.equalsIgnoreCase(VtnServiceConsts.EMPTY_STRING)) {
			try {
				isValid = (datapathId.substring(0, 2).equalsIgnoreCase("0X")) ? datapathId
						.matches(VtnServiceOpenStackConsts.OS_DATAPATH_ID_REGEX)
						: validator.isValidBigIntegerRangeString(new BigInteger(
								datapathId), VtnServiceJsonConsts.BIG_VAL0,
								VtnServiceJsonConsts.BIG_VAL_18446744073709551999);
			} catch (Exception e) {
				isValid = false;
			}
			if (isValid) {
				isValid = !datapathId
						.equalsIgnoreCase(VtnServiceOpenStackConsts.INVALID_DATA_PATH_ID);
			}
		}
		return isValid;
	}
}
