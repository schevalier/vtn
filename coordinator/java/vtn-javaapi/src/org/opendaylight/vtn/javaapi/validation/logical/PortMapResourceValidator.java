/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.javaapi.validation.logical;

import com.google.gson.JsonObject;
import org.opendaylight.vtn.core.util.Logger;
import org.opendaylight.vtn.javaapi.constants.VtnServiceConsts;
import org.opendaylight.vtn.javaapi.constants.VtnServiceJsonConsts;
import org.opendaylight.vtn.javaapi.exception.VtnServiceException;
import org.opendaylight.vtn.javaapi.ipc.enums.UncJavaAPIErrorCode;
import org.opendaylight.vtn.javaapi.resources.AbstractResource;
import org.opendaylight.vtn.javaapi.resources.logical.VBridgeInterfacePortMapResource;
import org.opendaylight.vtn.javaapi.resources.logical.VTepInterfacePortMapResource;
import org.opendaylight.vtn.javaapi.resources.logical.VTunnelInterfacePortMapResource;
import org.opendaylight.vtn.javaapi.validation.CommonValidator;
import org.opendaylight.vtn.javaapi.validation.VtnServiceValidator;

/**
 * The Class PortMapResourceValidator validates request Json object for PortMap
 * API.
 */
public class PortMapResourceValidator extends VtnServiceValidator {

	private static final Logger LOG = Logger
			.getLogger(PortMapResourceValidator.class.getName());

	private final AbstractResource resource;
	private final CommonValidator validator = new CommonValidator();

	/**
	 * Instantiates a new port map resource validator.
	 * 
	 * @param resource
	 *            the instance of AbstractResource
	 */
	public PortMapResourceValidator(final AbstractResource resource) {
		this.resource = resource;
	}

	/**
	 * Validate uri parameters for PortMap API.
	 * 
	 * @return true, if successful
	 */
	@Override
	public final boolean validateUri() {
		LOG.trace("Start PortMapResourceValidator#validateUri()");
		boolean isValid = false;
		setInvalidParameter(VtnServiceJsonConsts.URI
				+ VtnServiceJsonConsts.VTNNAME);
		if (resource instanceof VBridgeInterfacePortMapResource
				&& ((VBridgeInterfacePortMapResource) resource).getVtnName() != null
				&& !((VBridgeInterfacePortMapResource) resource).getVtnName()
						.trim().isEmpty()) {
			isValid = validator.isValidMaxLengthAlphaNum(
					((VBridgeInterfacePortMapResource) resource).getVtnName()
							.trim(), VtnServiceJsonConsts.LEN_31);
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.URI
						+ VtnServiceJsonConsts.VBRNAME);
				if (((VBridgeInterfacePortMapResource) resource).getVbrName() != null
						&& !((VBridgeInterfacePortMapResource) resource)
								.getVbrName().trim().isEmpty()) {
					isValid = validator.isValidMaxLengthAlphaNum(
							((VBridgeInterfacePortMapResource) resource)
									.getVbrName().trim(),
							VtnServiceJsonConsts.LEN_31);
				} else {
					isValid = false;
				}
			}
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.URI
						+ VtnServiceJsonConsts.IFNAME);
				if (((VBridgeInterfacePortMapResource) resource).getIfName() != null
						&& !((VBridgeInterfacePortMapResource) resource)
								.getIfName().trim().isEmpty()) {
					isValid = validator.isValidMaxLengthAlphaNum(
							((VBridgeInterfacePortMapResource) resource)
									.getIfName().trim(),
							VtnServiceJsonConsts.LEN_31);
				} else {
					isValid = false;
				}
			}
			setListOpFlag(false);
		} else if (resource instanceof VTunnelInterfacePortMapResource
				&& ((VTunnelInterfacePortMapResource) resource).getVtnName() != null
				&& !((VTunnelInterfacePortMapResource) resource).getVtnName()
						.trim().isEmpty()) {
			isValid = validator.isValidMaxLengthAlphaNum(
					((VTunnelInterfacePortMapResource) resource).getVtnName()
							.trim(), VtnServiceJsonConsts.LEN_31);
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.URI
						+ VtnServiceJsonConsts.VTUNNELNAME);
				if (((VTunnelInterfacePortMapResource) resource)
						.getVtunnelName() != null
						&& !((VTunnelInterfacePortMapResource) resource)
								.getVtunnelName().trim().isEmpty()) {
					isValid = validator.isValidMaxLengthAlphaNum(
							((VTunnelInterfacePortMapResource) resource)
									.getVtunnelName().trim(),
							VtnServiceJsonConsts.LEN_31);
				} else {
					isValid = false;
				}
			}
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.URI
						+ VtnServiceJsonConsts.IFNAME);
				if (((VTunnelInterfacePortMapResource) resource).getIfName() != null
						&& !((VTunnelInterfacePortMapResource) resource)
								.getIfName().trim().isEmpty()) {
					isValid = validator.isValidMaxLengthAlphaNum(
							((VTunnelInterfacePortMapResource) resource)
									.getIfName().trim(),
							VtnServiceJsonConsts.LEN_31);
				} else {
					isValid = false;
				}
			}
			setListOpFlag(false);
		} else if (resource instanceof VTepInterfacePortMapResource
				&& ((VTepInterfacePortMapResource) resource).getVtnName() != null
				&& !((VTepInterfacePortMapResource) resource).getVtnName()
						.trim().isEmpty()) {
			isValid = validator.isValidMaxLengthAlphaNum(
					((VTepInterfacePortMapResource) resource).getVtnName()
							.trim(), VtnServiceJsonConsts.LEN_31);
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.URI
						+ VtnServiceJsonConsts.VTEPNAME);
				if (((VTepInterfacePortMapResource) resource).getVtepName() != null
						&& !((VTepInterfacePortMapResource) resource)
								.getVtepName().trim().isEmpty()) {
					isValid = validator.isValidMaxLengthAlphaNum(
							((VTepInterfacePortMapResource) resource)
									.getVtepName().trim(),
							VtnServiceJsonConsts.LEN_31);
				} else {
					isValid = false;
				}
			}
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.URI
						+ VtnServiceJsonConsts.IFNAME);
				if (((VTepInterfacePortMapResource) resource).getIfName() != null
						&& !((VTepInterfacePortMapResource) resource)
								.getIfName().trim().isEmpty()) {
					isValid = validator.isValidMaxLengthAlphaNum(
							((VTepInterfacePortMapResource) resource)
									.getIfName().trim(),
							VtnServiceJsonConsts.LEN_31);
				} else {
					isValid = false;
				}
			}
			setListOpFlag(false);
		}
		LOG.trace("Completed PortMapResourceValidator#validateUri()");
		return isValid;
	}

	/**
	 * Validate request Json object for get, put method of PortMap API.
	 */
	@Override
	public final void
			validate(final String method, final JsonObject requestBody)
					throws VtnServiceException {
		LOG.trace("Start PortMapResourceValidator#validate()");
		LOG.info("Validating request for " + method
				+ " of PortMapResourceValidator");
		boolean isValid = false;
		try {
			isValid = validateUri();
			if (isValid && requestBody != null
					&& VtnServiceConsts.GET.equals(method)) {
				isValid = validator.isValidGet(requestBody, isListOpFlag());
				setInvalidParameter(validator.getInvalidParameter());
				updateOpParameterForList(requestBody);
			} else if (isValid && requestBody != null
					&& VtnServiceConsts.PUT.equals(method)) {
				isValid = validatePut(requestBody);
			} else if (isValid) {
				setInvalidParameter(VtnServiceConsts.INCORRECT_METHOD_INVOCATION);
				isValid = false;
			}
		} catch (final NumberFormatException e) {
			LOG.error("Inside catch:NumberFormatException");
			if (method.equals(VtnServiceConsts.GET)) {
				setInvalidParameter(validator.getInvalidParameter());
			}
			isValid = false;
		} catch (final ClassCastException e) {
			if (method.equals(VtnServiceConsts.GET)) {
				setInvalidParameter(validator.getInvalidParameter());
			}
			LOG.error("Inside catch:ClassCastException");
			isValid = false;
		}
		// Throws exception if validation fails
		if (!isValid) {
			LOG.error("Validation failed");
			throw new VtnServiceException(Thread.currentThread()
					.getStackTrace()[1].getMethodName(),
					UncJavaAPIErrorCode.VALIDATION_ERROR.getErrorCode(),
					UncJavaAPIErrorCode.VALIDATION_ERROR.getErrorMessage());
		}
		LOG.info("Validation successful");
		LOG.trace("Complete PortMapResourceValidator#validate()");

	}

	/**
	 * validate put request Json object for PortMap API.
	 * 
	 * @param requestBody
	 *            the request Json object
	 * @return true, if successful
	 */
	private boolean validatePut(final JsonObject requestBody) {
		LOG.trace("Start PortMapResourceValidator#validatePut()");
		boolean isValid = false;
		setInvalidParameter(VtnServiceJsonConsts.PORTMAP);
		if (requestBody.has(VtnServiceJsonConsts.PORTMAP)
				&& requestBody.get(VtnServiceJsonConsts.PORTMAP).isJsonObject()) {
			final JsonObject portMap = requestBody
					.getAsJsonObject(VtnServiceJsonConsts.PORTMAP);
			// validation for key: switch_id(mandatory)
			setInvalidParameter(VtnServiceJsonConsts.LOGICAL_PORT_ID);
			if (portMap.has(VtnServiceJsonConsts.LOGICAL_PORT_ID)
					&& portMap.getAsJsonPrimitive(
							VtnServiceJsonConsts.LOGICAL_PORT_ID).getAsString() != null
					&& !portMap
							.getAsJsonPrimitive(
									VtnServiceJsonConsts.LOGICAL_PORT_ID)
							.getAsString().trim().isEmpty()) {
				isValid = validator.isValidMaxLength(
						portMap.getAsJsonPrimitive(
								VtnServiceJsonConsts.LOGICAL_PORT_ID)
								.getAsString().trim(),
						VtnServiceJsonConsts.LEN_319);
			}
			if (isValid) {
				// validation for key: vlan_id
				setInvalidParameter(VtnServiceJsonConsts.VLANID);
				if (portMap.has(VtnServiceJsonConsts.VLANID)
						&& portMap.getAsJsonPrimitive(
								VtnServiceJsonConsts.VLANID).getAsString() != null) {
					isValid = validator.isValidRange(portMap
							.getAsJsonPrimitive(VtnServiceJsonConsts.VLANID)
							.getAsString().trim(), VtnServiceJsonConsts.VAL_1,
							VtnServiceJsonConsts.VAL_4095);
				}
			}
			// validation for key:tagged(optional)
			if (isValid) {
				setInvalidParameter(VtnServiceJsonConsts.TAGGED);
				if (portMap.has(VtnServiceJsonConsts.TAGGED)
						&& portMap.getAsJsonPrimitive(
								VtnServiceJsonConsts.TAGGED).getAsString() != null) {
					final String portMp = portMap
							.getAsJsonPrimitive(VtnServiceJsonConsts.TAGGED)
							.getAsString().trim();
					if (VtnServiceConsts.EMPTY_STRING.equals(portMp)) {
						return true;
					}
					isValid = portMp
							.equalsIgnoreCase(VtnServiceJsonConsts.TRUE)
							|| portMp
									.equalsIgnoreCase(VtnServiceJsonConsts.FALSE);
				}
			}
		}
		LOG.trace("Complete PortMapResourceValidator#validatePut()");
		return isValid;
	}
}
