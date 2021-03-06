/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.vtn.webapi.endpoint;

import java.io.IOException;

import javax.servlet.Servlet;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.json.JSONObject;

import com.google.gson.JsonObject;
import org.opendaylight.vtn.core.util.Logger;
import org.opendaylight.vtn.javaapi.init.VtnServiceInitManager;
import org.opendaylight.vtn.webapi.constants.ApplicationConstants;
import org.opendaylight.vtn.webapi.enums.ContentTypeEnum;
import org.opendaylight.vtn.webapi.enums.HttpErrorCodeEnum;
import org.opendaylight.vtn.webapi.exception.VtnServiceWebAPIException;
import org.opendaylight.vtn.webapi.services.VtnServiceWebAPIHandler;
import org.opendaylight.vtn.webapi.utils.ConfigurationManager;
import org.opendaylight.vtn.webapi.utils.DataConverter;
import org.opendaylight.vtn.webapi.utils.InitManager;
import org.opendaylight.vtn.webapi.utils.VtnServiceCommonUtil;
import org.opendaylight.vtn.webapi.utils.VtnServiceWebUtil;

/**
 * The Class VtnServiceWebAPIServle is the end point for all request coming to
 * Web API will be handled here first
 */
public class VtnServiceWebAPIServlet extends HttpServlet {

	/** The Constant serialVersionUID. */
	private static final long serialVersionUID = 9136213504159408769L;

	/** The Constant LOGGER. */
	private static final Logger LOG = Logger
			.getLogger(VtnServiceWebAPIServlet.class.getName());

	/**
	 * Initialize the HttpServlet and will initialize LOGGING and Configuration.
	 * 
	 * @param config
	 *            the config
	 * @see Servlet#init(ServletConfig)
	 */
	@Override
	public void init(final ServletConfig config) {
		LOG.trace("Start VtnServiceWebAPIServlet#init()");
		try {
			super.init();
			InitManager.initialize();
			LOG.info("Successful initialization of VTN Service");
		} catch (final Exception e) {
			LOG.error("Servlet Initialization failed error " + e.getMessage());
		}
		LOG.trace("Complete VtnServiceWebAPIServlet#init()");
	}

	/**
	 * Service Method will be called each time when a request will come to web
	 * API and it will validate the same for authentication and authorization
	 * After this will route the call to the specified method.
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @throws ServletException
	 *             the servlet exception
	 * @see HttpServlet#service(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	@Override
	protected void service(final HttpServletRequest request,
			final HttpServletResponse response) {
		LOG.trace("Start VtnServiceWebAPIServlet#service()");
		JSONObject serviceErrorJSON = null;
		VtnServiceWebAPIHandler vtnServiceWebAPIHandler = null;
		try {
			vtnServiceWebAPIHandler = new VtnServiceWebAPIHandler();
			vtnServiceWebAPIHandler.validate(request);
			super.service(request, response);
		} catch (final VtnServiceWebAPIException e) {
			VtnServiceCommonUtil.logErrorDetails(e.getErrorCode());
			LOG.error("VTN Service erorr occurred : " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil.prepareErrResponseJson(e
					.getErrorCode());
		} catch (final ClassCastException e) {
			LOG.error("Internal server error " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil
					.prepareErrResponseJson(HttpErrorCodeEnum.UNC_BAD_REQUEST
							.getCode());
		} catch (final Exception e) {
			LOG.error("Internal server error " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil
					.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
							.getCode());
		} finally {
			if (null != serviceErrorJSON) {
				try {
					createErrorResponse(request, response, serviceErrorJSON);
				} catch (IOException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (NumberFormatException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (final VtnServiceWebAPIException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(e.getErrorCode());
				}
			}
		}
		LOG.info("Successful Validation of VTN Service Request");
		LOG.trace("Complete VtnServiceWebAPIServlet#service()");
	}

	/**
	 * Do get.This method will get the json/xml data from java API and will
	 * write the same on servlet output
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @throws ServletException
	 *             the servlet exception
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	@Override
	protected void doGet(final HttpServletRequest request,
			final HttpServletResponse response) throws ServletException {
		LOG.trace("Start VtnServiceWebAPIServlet#doGet()");
		JSONObject serviceErrorJSON = null;
		VtnServiceWebAPIHandler vtnServiceWebAPIHandler = null;
		try {
			vtnServiceWebAPIHandler = new VtnServiceWebAPIHandler();
			final String responseString = vtnServiceWebAPIHandler.get(request);
			response.setStatus(HttpServletResponse.SC_OK);
			setResponseHeader(request, response, responseString,
					VtnServiceCommonUtil.getContentType(request));
		} catch (final IOException e) {
			serviceErrorJSON = VtnServiceWebUtil
					.prepareErrResponseJson(HttpErrorCodeEnum.UNC_BAD_REQUEST
							.getCode());
		} catch (final VtnServiceWebAPIException e) {
			LOG.error("VTN Service erorr occurred " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil.prepareErrResponseJson(e
					.getErrorCode());
		} finally {
			if (null != serviceErrorJSON) {
				try {
					createErrorResponse(request, response, serviceErrorJSON);
				} catch (IOException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (NumberFormatException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (final VtnServiceWebAPIException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(e.getErrorCode());
				}
			}
		}
		LOG.info("Successful processing for GET request.");
		LOG.trace("Complete VtnServiceWebAPIServlet#doGet()");
	}

	/**
	 * Do post.This method will be used to post the data either in xml or json
	 * to the java api by converting all the request to JSON and getting
	 * response in specified format.
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @throws ServletException
	 *             the servlet exception
	 * @throws IOException
	 *             Signals that an I/O exception has occurred.
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	@Override
	protected void doPost(final HttpServletRequest request,
			final HttpServletResponse response) throws ServletException,
			IOException {
		LOG.trace("Start VtnServiceWebAPIServlet#doPost()");
		JSONObject serviceErrorJSON = null;
		VtnServiceWebAPIHandler vtnServiceWebAPIHandler = null;
		try {
			vtnServiceWebAPIHandler = new VtnServiceWebAPIHandler();
			final String responseString = vtnServiceWebAPIHandler.post(request);
			response.setStatus(HttpServletResponse.SC_CREATED);
			setResponseHeader(request, response, responseString,
					VtnServiceCommonUtil.getContentType(request));
		} catch (final IOException e) {
			serviceErrorJSON = VtnServiceWebUtil
					.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
							.getCode());
		} catch (final VtnServiceWebAPIException e) {
			LOG.error("VTN Service erorr occurred " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil.prepareErrResponseJson(e
					.getErrorCode());
		} finally {
			if (null != serviceErrorJSON) {
				try {
					createErrorResponse(request, response, serviceErrorJSON);
				} catch (IOException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (NumberFormatException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (final VtnServiceWebAPIException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(e.getErrorCode());
				}
			}
		}
		LOG.info("Successful processing for POST request.");
		LOG.trace("Complete VtnServiceWebAPIServlet#doPost()");
	}

	/**
	 * Do put.This method will be used to post the data either in xml or json to
	 * the java api by converting all the request to JSON and getting response
	 * in specified format.
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @see HttpServlet#doPut(HttpServletRequest, HttpServletResponse)
	 */
	@Override
	protected void doPut(final HttpServletRequest request,
			final HttpServletResponse response) {
		LOG.trace("Start VtnServiceWebAPIServlet#init()");
		JSONObject serviceErrorJSON = null;
		VtnServiceWebAPIHandler vtnServiceWebAPIHandler = null;
		try {
			vtnServiceWebAPIHandler = new VtnServiceWebAPIHandler();
			final String responseString = vtnServiceWebAPIHandler.put(request);
			response.setStatus(HttpServletResponse.SC_NO_CONTENT);
			setResponseHeader(request, response, responseString,
					VtnServiceCommonUtil.getContentType(request));
		} catch (final IOException e) {
			serviceErrorJSON = VtnServiceWebUtil
					.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
							.getCode());
		} catch (final VtnServiceWebAPIException e) {
			LOG.error("VTN Service erorr occurred " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil.prepareErrResponseJson(e
					.getErrorCode());
		} finally {
			if (null != serviceErrorJSON) {
				try {
					createErrorResponse(request, response, serviceErrorJSON);
				} catch (IOException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (NumberFormatException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (final VtnServiceWebAPIException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(e.getErrorCode());
				}
			}
		}
		LOG.info("Successful processing for PUT request.");
		LOG.trace("Complete VtnServiceWebAPIServlet#doPut()");
	}

	/**
	 * Do delete.This method will be used to post the data either in xml or json
	 * to the java api by converting all the request to JSON and getting
	 * response in specified format.
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @see HttpServlet#doDelete(HttpServletRequest, HttpServletResponse)
	 */
	@Override
	protected void doDelete(final HttpServletRequest request,
			final HttpServletResponse response) {
		LOG.trace("Start VtnServiceWebAPIServlet#doDelete()");
		JSONObject serviceErrorJSON = null;
		VtnServiceWebAPIHandler vtnServiceWebAPIHandler = null;
		try {
			vtnServiceWebAPIHandler = new VtnServiceWebAPIHandler();
			final String responseString = vtnServiceWebAPIHandler
					.delete(request);
			response.setStatus(HttpServletResponse.SC_NO_CONTENT);
			setResponseHeader(request, response, responseString,
					VtnServiceCommonUtil.getContentType(request));
		} catch (final IOException e) {
			serviceErrorJSON = VtnServiceWebUtil
					.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
							.getCode());
		} catch (final VtnServiceWebAPIException e) {
			LOG.error("VTN Service erorr occurred " + e.getMessage());
			serviceErrorJSON = VtnServiceWebUtil.prepareErrResponseJson(e
					.getErrorCode());
		} finally {
			if (null != serviceErrorJSON) {
				try {
					createErrorResponse(request, response, serviceErrorJSON);
				} catch (IOException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (NumberFormatException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
									.getCode());
				} catch (final VtnServiceWebAPIException e) {
					LOG.error("VTN Service erorr occurred " + e.getMessage());
					serviceErrorJSON = VtnServiceWebUtil
							.prepareErrResponseJson(e.getErrorCode());
				}
			}
		}
		LOG.info("Successful processing for DELETE request.");
		LOG.trace("Complete VtnServiceWebAPIServlet#doDelete()");
	}

	/**
	 * destroy method to finalize Servlet instances and used objects in the Web
	 * API.
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @see HttpServlet#doPut(HttpServletRequest, HttpServletResponse)
	 */
	@Override
	public void destroy() {
		LOG.trace("Servlet instance is now eligible for garbage collection.");
		super.destroy();
		VtnServiceInitManager.getInstance().destroy();
	}

	/**
	 * Set response header information
	 * 
	 * @param request
	 *            the request
	 * @param response
	 *            the response
	 * @see HttpServlet#doPut(HttpServletRequest, HttpServletResponse)
	 */
	private void setResponseHeader(final HttpServletRequest request,
			final HttpServletResponse response, final String responseString,
			final String contentType) throws IOException,
			VtnServiceWebAPIException {
		LOG.trace("Start VtnServiceWebAPIServlet#setResponseHeader()");
		response.setContentType(contentType);
		response.setCharacterEncoding(ApplicationConstants.CHAR_ENCODING);
		if (null != responseString && !responseString.isEmpty()) {
			// Set response status in cases where returned response in not
			// success from JavaAPI
			final JsonObject responseJson = DataConverter
					.getConvertedRequestObject(responseString, contentType);
			if (responseJson.has(ApplicationConstants.ERROR)) {
				String actualErrorCode = responseJson
						.get(ApplicationConstants.ERROR).getAsJsonObject()
						.get(ApplicationConstants.ERR_CODE).getAsString();
				int errorCode = Integer.parseInt(actualErrorCode
						.substring(0, 3));

				/*
				 * If service is unavailable then retry-after header required to
				 * be set
				 */
				if (actualErrorCode.equalsIgnoreCase(String
						.valueOf(HttpErrorCodeEnum.UNC_SERVICE_UNAVAILABLE
								.getCode()))) {
					response.setHeader(
							ApplicationConstants.RETRY_AFTER,
							ConfigurationManager.getInstance().getConfProperty(
									ApplicationConstants.RETRY_AFTER));
				}

				LOG.debug("Set HTTP response status : " + errorCode);
				response.setStatus(errorCode);
				if (actualErrorCode.equalsIgnoreCase(String
						.valueOf(HttpErrorCodeEnum.UNC_CUSTOM_NOT_FOUND.getCode()))
						|| VtnServiceCommonUtil.isOpenStackResurce(request)) {

					errorCode = responseJson.get(ApplicationConstants.ERROR)
							.getAsJsonObject()
							.get(ApplicationConstants.ERR_CODE).getAsInt();
					if (errorCode == HttpErrorCodeEnum.UNC_CUSTOM_NOT_FOUND
							.getCode()) {
						errorCode = HttpErrorCodeEnum.UNC_NOT_FOUND.getCode();
						responseJson
								.get(ApplicationConstants.ERROR)
								.getAsJsonObject()
								.addProperty(
										ApplicationConstants.ERR_DESCRIPTION,
										HttpErrorCodeEnum.UNC_CUSTOM_NOT_FOUND
												.getMessage());
					}
					/**
					 * transform JSON object for error JSONs. err_code and
					 * err_msg parameters are not required to be nested inside
					 * error
					 */
					responseJson.addProperty(ApplicationConstants.ERR_CODE,
							String.valueOf(errorCode));

					/**
					 * changes the message for 500 error code UNC error message
					 * are not required to return to OpenStack API user
					 */
					if (actualErrorCode
							.equalsIgnoreCase(String
									.valueOf(HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
											.getCode()))) {
						responseJson.addProperty(
								ApplicationConstants.ERR_DESCRIPTION,
								HttpErrorCodeEnum.UNC_INTERNAL_SERVER_ERROR
										.getMessage());
					} else {
						responseJson
								.addProperty(
										ApplicationConstants.ERR_DESCRIPTION,
										responseJson
												.get(ApplicationConstants.ERROR)
												.getAsJsonObject()
												.get(ApplicationConstants.ERR_DESCRIPTION)
												.getAsString());
					}
					// error nested json is not required so remove the same
					responseJson.remove(ApplicationConstants.ERROR);

					response.getWriter().write(responseJson.toString());
				}
			} else {
				response.getWriter().write(responseString);
			}
		}
		LOG.info("Http Response Header Set Successfully");
		LOG.trace("Complete VtnServiceWebAPIServlet#setResponseHeader()");
	}

	/**
	 * Prepare error response and header in case of error conditions
	 * 
	 * @param request
	 * @param response
	 * @param serviceErrorJSON
	 * @throws VtnServiceWebAPIException
	 * @throws IOException
	 */
	private void createErrorResponse(final HttpServletRequest request,
			final HttpServletResponse response, JSONObject serviceErrorJSON)
			throws VtnServiceWebAPIException, IOException {
		String contentType = VtnServiceCommonUtil.getContentType(request);
		if (contentType == null
				|| !contentType
						.equalsIgnoreCase(ContentTypeEnum.APPLICATION_JSON
								.getContentType())
				|| !contentType
						.equalsIgnoreCase(ContentTypeEnum.APPLICATION_XML
								.getContentType())) {
			contentType = ContentTypeEnum.APPLICATION_JSON.getContentType();
		}
		final String responseString = DataConverter.getConvertedResponse(
				serviceErrorJSON, contentType);
		setResponseHeader(request, response, responseString, contentType);
	}

}
